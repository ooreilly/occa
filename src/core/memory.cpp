#include <occa/core/base.hpp>
#include <occa/core/memory.hpp>
#include <occa/core/device.hpp>
#include <occa/internal/core/device.hpp>
#include <occa/internal/core/memory.hpp>
#include <occa/internal/utils/sys.hpp>

namespace occa {
  memory::memory() :
      modeMemory(NULL) {}

  memory::memory(modeMemory_t *modeMemory_) :
      modeMemory(NULL) {
    setModeMemory(modeMemory_);
  }

  memory::memory(const memory &m) :
    modeMemory(NULL) {
    setModeMemory(m.modeMemory);
  }

  memory& memory::operator = (const memory &m) {
    setModeMemory(m.modeMemory);
    return *this;
  }

  memory::~memory() {
    removeMemoryRef();
  }

  void memory::assertInitialized() const {
    OCCA_ERROR("Memory not initialized or has been freed",
               modeMemory != NULL);
  }

  void memory::setModeMemory(modeMemory_t *modeMemory_) {
    if (modeMemory != modeMemory_) {
      removeMemoryRef();
      modeMemory = modeMemory_;
      if (modeMemory) {
        modeMemory->addMemoryRef(this);
      }
    }
  }

  void memory::removeMemoryRef() {
    if (!modeMemory) {
      return;
    }
    modeMemory->removeMemoryRef(this);
    if (modeMemory->modeMemory_t::needsFree()) {
      delete modeMemory;
      modeMemory = NULL;
    }
  }

  void memory::dontUseRefs() {
    if (modeMemory) {
      modeMemory->modeMemory_t::dontUseRefs();
    }
  }

  bool memory::isInitialized() const {
    return (modeMemory != NULL);
  }

  memory& memory::swap(memory &m) {
    modeMemory_t *modeMemory_ = modeMemory;
    modeMemory   = m.modeMemory;
    m.modeMemory = modeMemory_;
    return *this;
  }

  template <>
  void* memory::ptr<void>() {
    return (modeMemory
            ? modeMemory->getPtr()
            : NULL);
  }

  template <>
  const void* memory::ptr<void>() const {
    return (modeMemory
            ? modeMemory->getPtr()
            : NULL);
  }

  modeMemory_t* memory::getModeMemory() const {
    return modeMemory;
  }

  modeDevice_t* memory::getModeDevice() const {
    return modeMemory->getModeDevice();
  }

  occa::device memory::getDevice() const {
    return occa::device(modeMemory
                        ? modeMemory->getModeDevice()
                        : NULL);
  }

  memory::operator kernelArg() const {
    return kernelArg(modeMemory);
  }

  const std::string& memory::mode() const {
    static const std::string noMode = "No Mode";
    return (modeMemory
            ? modeMemory->getModeDevice()->mode
            : noMode);
  }

  const occa::json& memory::properties() const {
    static const occa::json noProperties;
    return (modeMemory
            ? modeMemory->properties()
            : noProperties);
  }

  void memory::setDtype(const dtype_t &dtype__) {
    assertInitialized();
    OCCA_ERROR("Memory dtype [" << dtype__.name() << "] must be registered",
               dtype__.isRegistered());
    modeMemory->dtype_ = &(dtype__.self());
  }

  const dtype_t& memory::dtype() const {
    if (modeMemory) {
      return *(modeMemory->dtype_);
    }
    return dtype::none;
  }

  udim_t memory::size() const {
    if (modeMemory == NULL) {
      return 0;
    }
    return modeMemory->size;
  }

  udim_t memory::length() const {
    if (modeMemory == NULL) {
      return 0;
    }
    return modeMemory->size / modeMemory->dtype_->bytes();
  }

  bool memory::operator == (const occa::memory &other) const {
    return (modeMemory == other.modeMemory);
  }

  bool memory::operator != (const occa::memory &other) const {
    return (modeMemory != other.modeMemory);
  }

  occa::memory memory::operator + (const dim_t offset) const {
    return slice(offset);
  }

  occa::memory& memory::operator += (const dim_t offset) {
    *this = slice(offset);
    return *this;
  }

  occa::memory memory::slice(const dim_t offset,
                             const dim_t count) const {
    assertInitialized();

    const int dtypeSize = modeMemory->dtype_->bytes();
    const dim_t offset_ = dtypeSize * offset;
    const dim_t bytes  = dtypeSize * ((count == -1)
                                      ? (length() - offset)
                                      : count);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= 0);

    OCCA_ERROR("Cannot have offset and bytes greater than the memory size ("
               << offset_ << " + " << bytes << " > " << size() << ")",
               (offset_ + (dim_t) bytes) <= (dim_t) size());

    occa::memory m(modeMemory->slice(offset_, bytes));
    m.setDtype(dtype());

    return m;
  }

  void memory::copyFrom(const void *src,
                        const dim_t bytes,
                        const dim_t offset,
                        const occa::json &props) {
    assertInitialized();

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << offset << ")",
               offset >= 0);

    OCCA_ERROR("Destination memory has size [" << modeMemory->size << "],"
               << " trying to access [" << offset << ", " << (offset + bytes_) << "]",
               (bytes_ + offset) <= modeMemory->size);

    modeMemory->copyFrom(src, bytes_, offset, props);
  }

  void memory::copyFrom(const memory src,
                        const dim_t bytes,
                        const dim_t destOffset,
                        const dim_t srcOffset,
                        const occa::json &props) {
    assertInitialized();

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << destOffset << ")",
               destOffset >= 0);

    OCCA_ERROR("Cannot have a negative offset (" << srcOffset << ")",
               srcOffset >= 0);

    OCCA_ERROR("Source memory has size [" << src.modeMemory->size << "],"
               << " trying to access [" << srcOffset << ", " << (srcOffset + bytes_) << "]",
               (bytes_ + srcOffset) <= src.modeMemory->size);

    OCCA_ERROR("Destination memory has size [" << modeMemory->size << "],"
               << " trying to access [" << destOffset << ", " << (destOffset + bytes_) << "]",
               (bytes_ + destOffset) <= modeMemory->size);

    modeMemory->copyFrom(src.modeMemory, bytes_, destOffset, srcOffset, props);
  }

  void memory::copyTo(void *dest,
                      const dim_t bytes,
                      const dim_t offset,
                      const occa::json &props) const {
    assertInitialized();

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << offset << ")",
               offset >= 0);

    OCCA_ERROR("Source memory has size [" << modeMemory->size << "],"
               << " trying to access [" << offset << ", " << (offset + bytes_) << "]",
               (bytes_ + offset) <= modeMemory->size);

    modeMemory->copyTo(dest, bytes_, offset, props);
  }

  void memory::copyTo(memory dest,
                      const dim_t bytes,
                      const dim_t destOffset,
                      const dim_t srcOffset,
                      const occa::json &props) const {
    assertInitialized();

    udim_t bytes_ = ((bytes == -1) ? modeMemory->size : bytes);

    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= -1);

    OCCA_ERROR("Cannot have a negative offset (" << destOffset << ")",
               destOffset >= 0);

    OCCA_ERROR("Cannot have a negative offset (" << srcOffset << ")",
               srcOffset >= 0);

    OCCA_ERROR("Source memory has size [" << modeMemory->size << "],"
               << " trying to access [" << srcOffset << ", " << (srcOffset + bytes_) << "]",
               (bytes_ + srcOffset) <= modeMemory->size);

    OCCA_ERROR("Destination memory has size [" << dest.modeMemory->size << "],"
               << " trying to access [" << destOffset << ", " << (destOffset + bytes_) << "]",
               (bytes_ + destOffset) <= dest.modeMemory->size);

    dest.modeMemory->copyFrom(modeMemory, bytes_, destOffset, srcOffset, props);
  }

  void memory::copyFrom(const void *src,
                        const occa::json &props) {
    copyFrom(src, -1, 0, props);
  }

  void memory::copyFrom(const memory src,
                        const occa::json &props) {
    copyFrom(src, -1, 0, 0, props);
  }

  void memory::copyTo(void *dest,
                      const occa::json &props) const {
    copyTo(dest, -1, 0, props);
  }

  void memory::copyTo(const memory dest,
                      const occa::json &props) const {
    copyTo(dest, -1, 0, 0, props);
  }

  occa::memory memory::cast(const dtype_t &dtype_) const {
    occa::memory mem = slice(0);
    mem.setDtype(dtype_);
    return mem;
  }

  occa::memory memory::clone() const {
    if (!modeMemory) {
      return occa::memory();
    }

    occa::memory mem = (
      occa::device(modeMemory->getModeDevice())
      .malloc(size(), *this, properties())
    );
    mem.setDtype(dtype());

    return mem;
  }

  void memory::free() {
    if (modeMemory == NULL) return;
    modeMemory->free();
  }

  void memory::detach() {
    if (modeMemory == NULL) return;
    modeMemory->detach();
  }

  memory null;

  std::ostream& operator << (std::ostream &out,
                             const occa::memory &memory) {
    out << memory.properties();
    return out;
  }
}
