#ifndef _QCC_KEYINFO_H
#define _QCC_KEYINFO_H
/**
 * @file
 *
 * This file provide public key info
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

#include <qcc/platform.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>

namespace qcc {

/**
 * KeyInfo
 */
class KeyInfo {

  public:

    /**
     * KeyInfo format
     */
    typedef enum {
        FORMAT_ALLJOYN = 0,   ///< AllJoyn format
        FORMAT_JWK = 1,   ///< JSON Web Key format
        FORMAT_X509 = 2,   ///< X.509 format
    } FormatType;

    /**
     * Key usage
     */
    typedef enum {
        USAGE_SIGNING = 0,   ///< key is used for signing
        USAGE_ENCRYPTION = 1   ///< key is used for encryption
    } KeyUsageType;

    /**
     * Default constructor.
     */
    KeyInfo(FormatType format) : format(format), keyIdLen(0), keyId(NULL)
    {
    }

    /**
     * Default destructor.
     */
    virtual ~KeyInfo()
    {
        delete [] keyId;
    }

    /**
     * Assign the key id
     * @param keyID the key id to copy
     * @param len the key len
     */
    void SetKeyId(const uint8_t* keyID, size_t len)
    {
        delete [] keyId;
        keyId = NULL;
        keyIdLen = 0;
        if (len == 0) {
            return;
        }
        keyId = new uint8_t[len];
        if (keyId == NULL) {
            return;
        }
        keyIdLen = len;
        memcpy(keyId, keyID, keyIdLen * sizeof(uint8_t));
    }

    /**
     * Retrieve the key ID.
     * @return  the key ID.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const uint8_t* GetKeyId() const
    {
        return keyId;
    }

    /**
     * Retrieve the key ID length.
     * @return  the key ID length.
     */
    const size_t GetKeyIdLen() const
    {
        return keyIdLen;
    }

    /**
     * The required size of the exported byte array.
     * @return the required size of the exported byte array
     */

    const size_t GetExportSize() const;

    /**
     * Export the KeyInfo data to a byte array.
     * @param[in,out] buf the pointer to a byte array.  The caller must allocateenough memory based on call GetExportSize().
     * @return ER_OK for success; otherwise, an error code
     */

    QStatus Export(uint8_t* buf) const;

    /**
     * Import a byte array generated by a KeyInfo Export.
     * @param buf the export data
     * @param count the size of the export data
     * @return ER_OK for success; otherwise, an error code
     */

    QStatus Import(const uint8_t* buf, size_t count);

    /**
     * Get the format
     * @return the format
     */
    const FormatType GetFormat() const
    {
        return format;
    }

    /**
     * Comparison operators equality
     * @param[in] other right hand side KeyInfo
     * @return true is keys are equal.
     */
    bool operator==(const KeyInfo& other) const
    {
        if (format != other.format) {
            return false;
        }

        if (keyIdLen != other.keyIdLen) {
            return false;
        }

        if (keyId == NULL || other.keyId == NULL) {
            return keyId == other.keyId;
        }

        if (0 != memcmp(keyId, other.keyId, keyIdLen)) {
            return false;
        }

        return true;
    }

    /**
     * Comparison operators non-equality
     * @param[in] other right hand side KeyInfo
     * @return true is keys are not equal
     */
    bool operator!=(const KeyInfo& other) const
    {
        return !(*this == other);
    }

    /**
     * Assignment operator is private
     */
    KeyInfo& operator=(const KeyInfo& other) {
        if (this != &other) {
            format = other.format;
            keyIdLen = other.keyIdLen;
            delete [] keyId;
            keyId = new uint8_t[keyIdLen];
            memcpy(keyId, other.keyId, keyIdLen);
        }
        return *this;
    }

    /**
     * Comparison operator less than
     * @param[in] other right hand side KeyInfo
     * @return true if key is less than the other
     */
    bool operator<(const KeyInfo& other) const
    {
        if (format < other.format) {
            return true;
        }

        if (keyIdLen < other.keyIdLen) {
            return true;
        }

        if (keyId != NULL && other.keyId != NULL && keyIdLen == other.keyIdLen) {
            if (0 > memcmp(keyId, other.keyId, keyIdLen)) {
                return true;
            }
        }

        return false;
    }

    /**
     * Copy constructor for KeyInfo
     */
    KeyInfo(const KeyInfo& other) {
        format = other.format;
        keyIdLen = other.keyIdLen;
        keyId = new uint8_t[keyIdLen];
        memcpy(keyId, other.keyId, keyIdLen);
    }

  private:

    FormatType format;
    size_t keyIdLen;
    uint8_t* keyId;
};

} /* namespace qcc */


#endif
