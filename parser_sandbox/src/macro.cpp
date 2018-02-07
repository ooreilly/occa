/* The MIT License (MIT)
 *
 * Copyright (c) 2014-2018 David Medina and Tim Warburton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */
#include <cstring>

#include "occa/defines.hpp"
#include "occa/types.hpp"
#include "occa/tools/lex.hpp"
#include "occa/tools/string.hpp"
#include "occa/tools/sys.hpp"

#include "macro.hpp"
#include "preprocessor.hpp"

namespace occa {
  namespace lang {
    const std::string macro_t::VA_ARGS = "__VA_ARGS__";

    macroToken::macroToken() :
      token(NULL),
      arg(-1) {}

    macroToken::macroToken(token_t *token_) :
      token(token_),
      arg(-1) {}

    macroToken::macroToken(const int arg_) :
      token(NULL),
      arg(arg_) {}

    macro_t::macro_t(tokenStream *sourceStream_,
                     const std::string &name_) :
      name(name_),
      argCount(-1),
      hasVarArgs(false),
      macroTokenIndex(0),
      argTokenIndex(0) {
      sourceStream = sourceStream_;
    }

    macro_t::~macro_t() {
      const int tokens = (int) macroTokens.size();
      for (int i = 0; i < tokens; ++i) {
        delete macroTokens[i].token;
      }
      macroTokens.clear();
    }

    bool macro_t::loadArgs() {
      macroTokenIndex = 0;
      argTokenIndex = 0;

      if (!isFunctionLike()) {
        return true;
      }

      // Assumes ( has been checked
      int argIndex = 0;
      while (true) {
        token_t *token = getSourceToken();
        if (!token) {
          printError("Not able to find closing )");
          break;
        }
        if (token->type() == tokenType::op) {
          opType_t opType = token->to<operatorToken>().op.opType;
          // Check for closing )
          if (opType == operatorType::parenthesesEnd) {
            return true;
          }
          // Check for comma
          if (opType == operatorType::comma) {
            ++argIndex;
            // Make sure we haven't passed arg count
            if (!hasVarArgs && (argIndex >= argCount)) {
              std::stringstream ss;
              if (argCount) {
                ss << "Too many arguments, expected "
                   << argCount << " argument";
                if (argCount > 1) {
                  ss << 's';
                }
              } else {
                ss << "Macro does not take arguments";
              }
              printError(ss.str());
              break;
            }
            args.push_back(tokenVector());
            continue;
          }
        }
        // Add token to current arg
        args[argIndex].push_back(token);
      }
      return false;
    }

    token_t* macro_t::_getToken() {
      const int tokens = (int) macroTokens.size();

      while (macroTokenIndex < tokens) {
        macroToken token = macroTokens[macroTokenIndex];
        if (token.arg < 0) {
          ++macroTokenIndex;
          return token.token->clone();
        }
        tokenVector &argTokens = args[token.arg];
        const int argTokenCount = (int) argTokens.size();
        if (argTokenIndex < argTokenCount) {
          return argTokens[argTokenIndex++];
        } else {
          ++macroTokenIndex;
          argTokenIndex = 0;
        }
      }

      return NULL;
    }
  }
}
