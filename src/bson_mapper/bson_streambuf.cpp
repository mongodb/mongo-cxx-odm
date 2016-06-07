// Copyright 2016 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <streambuf>

#include <bson.h>
#include "bson_streambuf.hpp"

#include <bson_mapper/config/prelude.hpp>

namespace bson_mapper {
BSON_MAPPER_INLINE_NAMESPACE_BEGIN

bson_output_streambuf::bson_output_streambuf(mongocxx::collection coll) : coll(coll) {
}

bson_output_streambuf::~bson_output_streambuf() {
    if (data != nullptr) {
        delete[] data;
    }
}

int bson_output_streambuf::underflow() {
    return EOF;
}

int bson_output_streambuf::overflow(int ch) {
    int result = EOF;
    if (ch < 0 || ch > std::numeric_limits<uint8_t>::max()) {
        throw std::invalid_argument("Character is out of bounds.");
    }
    assert(ch >= 0 && ch <= std::numeric_limits<uint8_t>::max());
    result = insert(ch);
    return result;
}

int bson_output_streambuf::insert(int ch) {
    bytes_read++;

    // For the first four bytes, this builds int32 that contains the document size.
    if (bytes_read <= 4) {
        len |= (ch << (8 * (bytes_read - 1)));
    }
    // Once the document size is received, allocate space.
    if (bytes_read == 4) {
        if (len > BSON_MAX_SIZE) {
            throw std::invalid_argument("BSON document length is too large.");
        }
        data = new uint8_t[len];
        std::memcpy(data, &len, 4);
    }

    if (bytes_read > 4) {
        data[bytes_read - 1] = static_cast<uint8_t>(ch);
    }

    if (bytes_read == len) {
        // This inserts the document into the collection, and resets the data and length variables.
        bsoncxx::document::view v(data, len);
        coll.insert_one(v);
        delete[] data;
        data = nullptr;
        bytes_read = 0;
        len = 0;
    } else if (bytes_read > len && len >= 4) {
        return EOF;
    }
    return ch;
}

BSON_MAPPER_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <bson_mapper/config/postlude.hpp>
