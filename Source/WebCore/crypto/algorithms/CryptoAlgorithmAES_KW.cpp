/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CryptoAlgorithmAES_KW.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "CryptoAlgorithmAesKeyGenParams.h"
#include "CryptoAlgorithmAesKeyGenParamsDeprecated.h"
#include "CryptoKeyAES.h"
#include "CryptoKeyDataOctetSequence.h"
#include "ExceptionCode.h"

namespace WebCore {

static inline bool usagesAreInvalidForCryptoAlgorithmAES_KW(CryptoKeyUsageBitmap usages)
{
    return usages & (CryptoKeyUsageSign | CryptoKeyUsageVerify | CryptoKeyUsageDeriveKey | CryptoKeyUsageDeriveBits | CryptoKeyUsageEncrypt | CryptoKeyUsageDecrypt);
}

Ref<CryptoAlgorithm> CryptoAlgorithmAES_KW::create()
{
    return adoptRef(*new CryptoAlgorithmAES_KW);
}

CryptoAlgorithmIdentifier CryptoAlgorithmAES_KW::identifier() const
{
    return s_identifier;
}

bool CryptoAlgorithmAES_KW::keyAlgorithmMatches(const CryptoKey& key) const
{
    if (key.algorithmIdentifier() != s_identifier)
        return false;
    ASSERT(is<CryptoKeyAES>(key));
    return true;
}

void CryptoAlgorithmAES_KW::generateKey(const std::unique_ptr<CryptoAlgorithmParameters>&& parameters, bool extractable, CryptoKeyUsageBitmap usages, KeyOrKeyPairCallback&& callback, ExceptionCallback&& exceptionCallback, ScriptExecutionContext&)
{
    if (usagesAreInvalidForCryptoAlgorithmAES_KW(usages)) {
        exceptionCallback(SYNTAX_ERR);
        return;
    }

    auto result = CryptoKeyAES::generate(CryptoAlgorithmIdentifier::AES_KW, downcast<CryptoAlgorithmAesKeyGenParams>(*parameters).length, extractable, usages);
    if (!result) {
        exceptionCallback(OperationError);
        return;
    }

    callback(result.get(), nullptr);
}

void CryptoAlgorithmAES_KW::importKey(SubtleCrypto::KeyFormat format, KeyData&& data, const std::unique_ptr<CryptoAlgorithmParameters>&& parameters, bool extractable, CryptoKeyUsageBitmap usages, KeyCallback&& callback, ExceptionCallback&& exceptionCallback)
{
    if (usagesAreInvalidForCryptoAlgorithmAES_KW(usages)) {
        exceptionCallback(SYNTAX_ERR);
        return;
    }

    RefPtr<CryptoKeyAES> result;
    switch (format) {
    case SubtleCrypto::KeyFormat::Raw:
        result = CryptoKeyAES::importRaw(parameters->identifier, WTFMove(WTF::get<Vector<uint8_t>>(data)), extractable, usages);
        break;
    case SubtleCrypto::KeyFormat::Jwk: {
        result = CryptoKeyAES::importJwk(parameters->identifier, WTFMove(WTF::get<JsonWebKey>(data)), extractable, usages, [](size_t length, const Optional<String>& alg) -> bool {
            switch (length) {
            case CryptoKeyAES::s_length128:
                return !alg || alg.value() == "A128KW";
            case CryptoKeyAES::s_length192:
                return !alg || alg.value() == "A192KW";
            case CryptoKeyAES::s_length256:
                return !alg || alg.value() == "A256KW";
            }
            return false;
        });
        break;
    }
    default:
        exceptionCallback(NOT_SUPPORTED_ERR);
        return;
    }
    if (!result) {
        exceptionCallback(DataError);
        return;
    }

    callback(*result);
}

ExceptionOr<void> CryptoAlgorithmAES_KW::encryptForWrapKey(const CryptoAlgorithmParametersDeprecated&, const CryptoKey& key, const CryptoOperationData& data, VectorCallback&& callback, VoidCallback&& failureCallback)
{
    if (!keyAlgorithmMatches(key))
        return Exception { NOT_SUPPORTED_ERR };
    return platformEncrypt(downcast<CryptoKeyAES>(key), data, WTFMove(callback), WTFMove(failureCallback));
}

ExceptionOr<void> CryptoAlgorithmAES_KW::decryptForUnwrapKey(const CryptoAlgorithmParametersDeprecated&, const CryptoKey& key, const CryptoOperationData& data, VectorCallback&& callback, VoidCallback&& failureCallback)
{
    if (!keyAlgorithmMatches(key))
        return Exception { NOT_SUPPORTED_ERR };
    return platformDecrypt(downcast<CryptoKeyAES>(key), data, WTFMove(callback), WTFMove(failureCallback));
}

ExceptionOr<void> CryptoAlgorithmAES_KW::generateKey(const CryptoAlgorithmParametersDeprecated& parameters, bool extractable, CryptoKeyUsageBitmap usages, KeyOrKeyPairCallback&& callback, VoidCallback&& failureCallback, ScriptExecutionContext&)
{
    auto result = CryptoKeyAES::generate(CryptoAlgorithmIdentifier::AES_KW, downcast<CryptoAlgorithmAesKeyGenParamsDeprecated>(parameters).length, extractable, usages);
    if (!result) {
        failureCallback();
        return { };
    }
    callback(result.get(), nullptr);
    return { };
}

ExceptionOr<void> CryptoAlgorithmAES_KW::importKey(const CryptoAlgorithmParametersDeprecated&, const CryptoKeyData& keyData, bool extractable, CryptoKeyUsageBitmap usage, KeyCallback&& callback, VoidCallback&&)
{
    if (!is<CryptoKeyDataOctetSequence>(keyData))
        return Exception { NOT_SUPPORTED_ERR };
    callback(CryptoKeyAES::create(CryptoAlgorithmIdentifier::AES_KW, downcast<CryptoKeyDataOctetSequence>(keyData).octetSequence(), extractable, usage));
    return { };
}

}

#endif // ENABLE(SUBTLE_CRYPTO)
