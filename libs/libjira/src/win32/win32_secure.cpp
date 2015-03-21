/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"

#include "jira/server.hpp"
#include "wincrypt.h"

namespace jira {
	namespace secure {
		bool crypt(const std::vector<uint8_t>& plain, std::vector<uint8_t>& secret)
		{
			DATA_BLOB dataIn;
			DATA_BLOB dataOut;

			dataIn.pbData = (BYTE*) plain.data();
			dataIn.cbData = plain.size();

			if (!CryptProtectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut))
				return false;

			secret.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
			LocalFree(dataOut.pbData);
			return true;
		}

		bool decrypt(const std::vector<uint8_t>& secret, std::vector<uint8_t>& plain)
		{
			DATA_BLOB dataIn;
			DATA_BLOB dataOut;

			dataIn.pbData = (BYTE*) secret.data();
			dataIn.cbData = secret.size();

			if (!CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut))
				return false;

			plain.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
			LocalFree(dataOut.pbData);
			return true;
		}
	}
};
