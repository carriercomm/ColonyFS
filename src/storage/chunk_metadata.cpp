/*
 * chunk.cpp
 *
 *  Created on: Jul 15, 2008
 *      Author: vjeko
 */

#include "chunk_metadata.hpp"

namespace colony {

namespace storage {

chunk_metadata::chunk_metadata() :
    basic_metadata::basic_metadata("NONE", 1) {}

chunk_metadata::chunk_metadata(
    uid_type uid,
    cuid_type cuid) :
      basic_metadata::basic_metadata(uid, cuid) {}

chunk_metadata::chunk_metadata(key_type key) :
    basic_metadata(key) {}

chunk_metadata::~chunk_metadata() {}

}

}
