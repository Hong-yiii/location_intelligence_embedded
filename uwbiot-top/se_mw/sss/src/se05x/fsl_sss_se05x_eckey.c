/* Copyright 2019,2020 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file */

#if defined(FLOW_VERBOSE)
#define NX_LOG_ENABLE_SCP_DEBUG 1
#endif

#include <fsl_sss_se05x_apis.h>

#if SSS_HAVE_APPLET_SE05X_IOT
#if SSS_HAVE_SCP_SCP03_SSS && SSSFTR_SE05X_AuthECKey

#include <fsl_sss_se05x_scp03.h>
#include <nxEnsure.h>
#include <nxLog_scp.h>
#include <nxScp03_Apis.h>
#include <se05x_tlv.h>
#include <smCom.h>
#include <sm_const.h>
#include <string.h>
#if SSS_HAVE_HOSTCRYPTO_MBEDTLS
#include "fsl_sss_mbedtls_types.h"
#elif SSS_HAVE_HOSTCRYPTO_OPENSSL
#include "fsl_sss_openssl_types.h"
#endif

/* ************************************************************************** */
/* Functions : Private function declaration                                   */
/* ************************************************************************** */

static sss_status_t nxECKey_InternalAuthenticate(pSe05xSession_t se05xSession,
    SE05x_AuthCtx_ECKey_t *pAuthFScp,
    uint8_t *hostEckaPubKey,
    size_t hostEckaPubKeyLen,
    uint8_t *rndData,
    size_t *rndDataLen,
    uint8_t *receipt,
    size_t *receiptLen);

static sss_status_t nxECKey_calculate_master_secret(
    SE05x_AuthCtx_ECKey_t *pAuthFScp, uint8_t *rnd, size_t rndLen, uint8_t *sharedSecret, size_t sharedSecretLen);

static sss_status_t nxECKey_HostLocal_CalculateSessionKeys(SE05x_AuthCtx_ECKey_t *pAuthFScp);

static sss_status_t nxECKey_Calculate_Initial_Mac_Chaining_Value(SE05x_AuthCtx_ECKey_t *pAuthFScp);

static sss_status_t nxECKey_Calculate_Shared_secret(
    SE05x_AuthCtx_ECKey_t *pAuthFScp, uint8_t *sharedSecret, size_t *sharedSecretLen);

#define TAG_PK_SE_ECKA 0x7F49
#define TAG_SIG_SE_ECKA 0x5F37

static void set_secp256r1nist_header(uint8_t *pbKey, size_t *pbKeyByteLen);

int get_u8buf_2bTag(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, uint16_t tag, uint8_t *rsp, size_t *pRspLen);

/* ************************************************************************** */
/* Functions : Function definition                                            */
/* ************************************************************************** */

sss_status_t nxECKey_AuthenticateChannel(
    pSe05xSession_t se05xSession, SE05x_AuthCtx_ECKey_t *pAuthFScp, uint8_t *pSePubkey, size_t *pSePubkeyLen)
{
    sss_status_t status = kStatus_SSS_Fail;
    // Host public key to send to the SE for internal authenticate
    uint8_t hostEckaPub[100];
    size_t hostEckaPubLen = sizeof(hostEckaPub);
    size_t hostEckabitLen;
    // Random bytes to retrive from SE in internal authenticate
    uint8_t drSE[20];
    size_t drSELen = sizeof(drSE);
    uint8_t receipt[16];
    size_t receiptLen = sizeof(receipt);
    uint8_t shsSecret[32];
    size_t shsSecretLen                = sizeof(shsSecret);
    size_t offset                      = 0;
    NXECKey03_StaticCtx_t *pStatic_ctx = pAuthFScp->pStatic_ctx;
    NXSCP03_DynCtx_t *pDyn_ctx         = pAuthFScp->pDyn_ctx;
    uint8_t *pkSeEcka;

    /* clang-format off */
    const uint8_t commandCounter[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    /* clang-format on */

    /* Get the Host ephemeral key */
    uint8_t hostPubkey[100];
    status = sss_host_key_store_get_key(
        pStatic_ctx->HostEcKeypair.keyStore, &pStatic_ctx->HostEcKeypair, hostPubkey, &hostEckaPubLen, &hostEckabitLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);

    /* Get Ecc public key in the plain form required for Security Storage
    * according to GPCS Ammendment E For ECC Nist 256 key
    */
    hostEckaPub[offset++] = GPCS_KEY_TYPE_ECC_NIST256; //Tag EC public key
    hostEckaPub[offset++] = 0x41;                      // public key len
    if (hostEckaPubLen < ASN_ECC_NIST_256_HEADER_LEN) {
        status = kStatus_SSS_Fail;
        goto exit;
    }
    memcpy(
        hostEckaPub + offset, hostPubkey + ASN_ECC_NIST_256_HEADER_LEN, hostEckaPubLen - ASN_ECC_NIST_256_HEADER_LEN);
    if ((UINT_MAX - offset) < (hostEckaPubLen - ASN_ECC_NIST_256_HEADER_LEN)) {
        goto exit;
    }
    offset += hostEckaPubLen - ASN_ECC_NIST_256_HEADER_LEN;

    ENSURE_OR_GO_EXIT(offset + 3 <= hostEckaPubLen);
    hostEckaPub[offset++] = KEY_PARAMETER_REFERENCE_TAG;
    hostEckaPub[offset++] = KEY_PARAMETER_REFERENCE_VALUE_LEN;
    hostEckaPub[offset++] = KEY_PARAMETER_REFERENCE_VALUE;
    hostEckaPubLen        = offset;

    pkSeEcka = pSePubkey; // Exclude first two bytes Tag and len
    set_secp256r1nist_header(pkSeEcka, pSePubkeyLen);

    status = sss_host_key_store_set_key(
        pStatic_ctx->SeEcPubKey.keyStore, &pStatic_ctx->SeEcPubKey, pkSeEcka, *pSePubkeyLen, 256, NULL, 0);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);

    status = nxECKey_InternalAuthenticate(
        se05xSession, pAuthFScp, hostEckaPub, hostEckaPubLen, drSE, &drSELen, receipt, &receiptLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);

    /*Calculate the Shared Secret */
    status = nxECKey_Calculate_Shared_secret(pAuthFScp, shsSecret, &shsSecretLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);

    /*Erase the host key pair as it is no longer needed*/
    memset(hostEckaPub, 0, sizeof(hostEckaPub));
    memset(hostPubkey, 0, sizeof(hostPubkey));
    sss_key_object_free(&pStatic_ctx->HostEcKeypair);

    status = nxECKey_calculate_master_secret(pAuthFScp, drSE, drSELen, shsSecret, shsSecretLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);

    status = nxECKey_HostLocal_CalculateSessionKeys(pAuthFScp);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);
    /* Increment the command Encreption counter to 1*/
    memcpy(pDyn_ctx->cCounter, commandCounter, AES_KEY_LEN_nBYTE);

    /* compute the initial MAC chaining value */
    status = nxECKey_Calculate_Initial_Mac_Chaining_Value(pAuthFScp);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);
    pDyn_ctx->SecurityLevel = (uint8_t)SECURITY_LEVEL;
exit:
    return status;
}

static sss_status_t nxECKey_Calculate_Initial_Mac_Chaining_Value(SE05x_AuthCtx_ECKey_t *pAuthFScp)
{
    sss_status_t status = kStatus_SSS_Fail;
    uint8_t ddA[128];
    uint16_t ddALen                           = sizeof(ddA);
    uint8_t iniMacChaining[AES_KEY_LEN_nBYTE] = {0};
    uint32_t signatureLen                     = AES_KEY_LEN_nBYTE;
    NXECKey03_StaticCtx_t *pStatic_ctx        = pAuthFScp->pStatic_ctx;
    NXSCP03_DynCtx_t *pDyn_ctx                = pAuthFScp->pDyn_ctx;

    // Set the Derviation data
    nxScp03_setDerivationData(
        ddA, &ddALen, DATA_DERIVATION_INITIAL_MCV, DATA_DERIVATION_L_128BIT, DATA_DERIVATION_KDF_CTR, NULL, 0);
    // Calculate the Initial MCV value
    status = nxScp03_Generate_SessionKey(&pStatic_ctx->masterSec, ddA, ddALen, iniMacChaining, &signatureLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);
    LOG_MAU8_D("Initial MCV", iniMacChaining, AES_KEY_LEN_nBYTE);
    // Set the Initial MCV value
    memcpy(pDyn_ctx->MCV, iniMacChaining, AES_KEY_LEN_nBYTE);
exit:
    return status;
}

static sss_status_t nxECKey_HostLocal_CalculateSessionKeys(SE05x_AuthCtx_ECKey_t *pAuthFScp)
{
    sss_status_t status = kStatus_SSS_Fail;
    uint8_t ddA[128];
    uint16_t ddALen                           = sizeof(ddA);
    uint8_t sessionEncKey[AES_KEY_LEN_nBYTE]  = {0};
    uint8_t sessionMacKey[AES_KEY_LEN_nBYTE]  = {0};
    uint8_t sessionRmacKey[AES_KEY_LEN_nBYTE] = {0};
    uint32_t signatureLen                     = AES_KEY_LEN_nBYTE;
    NXECKey03_StaticCtx_t *pStatic_ctx        = pAuthFScp->pStatic_ctx;
    NXSCP03_DynCtx_t *pDyn_ctx                = pAuthFScp->pDyn_ctx;

    /* Generation and Creation of Session ENC SSS Key Object */

    // Set the Derviation data
    nxScp03_setDerivationData(
        ddA, &ddALen, DATA_DERIVATION_SENC, DATA_DERIVATION_L_128BIT, DATA_DERIVATION_KDF_CTR, NULL, 0);
    // Calculate the Session-ENC key
    status = nxScp03_Generate_SessionKey(&pStatic_ctx->masterSec, ddA, ddALen, sessionEncKey, &signatureLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);
    LOG_MAU8_D("sessionEncKey", sessionEncKey, AES_KEY_LEN_nBYTE);
    // Set the Session-ENC key
    status = sss_host_key_store_set_key(pDyn_ctx->Enc.keyStore, &pDyn_ctx->Enc, sessionEncKey, 16, (16) * 8, NULL, 0);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);

    /* Generation and Creation of Session MAC SSS Key Object */

    // Set the Derviation data
    nxScp03_setDerivationData(
        ddA, &ddALen, DATA_DERIVATION_SMAC, DATA_DERIVATION_L_128BIT, DATA_DERIVATION_KDF_CTR, NULL, 0);
    // Calculate the Session-MAC key
    status = nxScp03_Generate_SessionKey(&pStatic_ctx->masterSec, ddA, ddALen, sessionMacKey, &signatureLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);
    LOG_MAU8_D("sessionMacKey", sessionMacKey, AES_KEY_LEN_nBYTE);
    // Set the Session-MAC key
    status = sss_host_key_store_set_key(pDyn_ctx->Mac.keyStore, &pDyn_ctx->Mac, sessionMacKey, 16, (16) * 8, NULL, 0);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);

    /* Generation and Creation of Session RMAC SSS Key Object */

    // Set the Derviation data
    nxScp03_setDerivationData(
        ddA, &ddALen, DATA_DERIVATION_SRMAC, DATA_DERIVATION_L_128BIT, DATA_DERIVATION_KDF_CTR, NULL, 0);
    // Calculate the Session-RMAC key
    status = nxScp03_Generate_SessionKey(&pStatic_ctx->masterSec, ddA, ddALen, sessionRmacKey, &signatureLen);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);
    LOG_MAU8_D("sessionRmacKey", sessionRmacKey, AES_KEY_LEN_nBYTE);
    // Set the Session-RMAC key
    status =
        sss_host_key_store_set_key(pDyn_ctx->Rmac.keyStore, &pDyn_ctx->Rmac, sessionRmacKey, 16, (16) * 8, NULL, 0);
    ENSURE_OR_GO_EXIT(status == kStatus_SSS_Success);
exit:
    return status;
}

static sss_status_t nxECKey_calculate_master_secret(
    SE05x_AuthCtx_ECKey_t *pAuthFScp, uint8_t *rnd, size_t rndLen, uint8_t *sharedSecret, size_t sharedSecretLen)
{
    sss_status_t status = kStatus_SSS_Fail;
    sss_digest_t md;
    uint8_t derivationInput[100] = {0};
    uint8_t masterSk[32];
    size_t masterSkLen                 = sizeof(masterSk);
    size_t derivationInputLen          = 0;
    NXECKey03_StaticCtx_t *pStatic_ctx = pAuthFScp->pStatic_ctx;

    if (pAuthFScp->pDyn_ctx->authType == kSSS_AuthType_INT_ECKey_Counter) {
        const uint8_t kdf_counter[] = {0x00, 0x00, 0x00, 0x01};
        ENSURE_OR_GO_CLEANUP(sizeof(kdf_counter) <= sizeof(derivationInput) - derivationInputLen);
        memcpy(&derivationInput[derivationInputLen], kdf_counter, sizeof(kdf_counter));
        derivationInputLen += sizeof(kdf_counter);
    }

    ENSURE_OR_GO_CLEANUP(sharedSecretLen <= sizeof(derivationInput) - derivationInputLen);
    memcpy(&derivationInput[derivationInputLen], sharedSecret, sharedSecretLen);

    if ((UINT_MAX - derivationInputLen) < sharedSecretLen) {
        goto cleanup;
    }
    derivationInputLen += sharedSecretLen;
    ENSURE_OR_GO_CLEANUP(rndLen <= (sizeof(derivationInput) - derivationInputLen));
    memcpy(&derivationInput[derivationInputLen], rnd, rndLen);

    if ((UINT_MAX - derivationInputLen) < rndLen) {
        goto cleanup;
    }
    derivationInputLen += rndLen;

    ENSURE_OR_GO_CLEANUP(derivationInputLen <= (sizeof(derivationInput) - 4))
    derivationInput[derivationInputLen++] = SCP_CONFIG;
    derivationInput[derivationInputLen++] = SECURITY_LEVEL;
    derivationInput[derivationInputLen++] = GPCS_KEY_TYPE_AES;
    derivationInput[derivationInputLen++] = GPCS_KEY_LEN_AES;

    status = sss_host_digest_context_init(
        &md, pStatic_ctx->HostEcdsaObj.keyStore->session, kAlgorithm_SSS_SHA256, kMode_SSS_Digest);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    status = sss_host_digest_one_go(&md, derivationInput, derivationInputLen, masterSk, &masterSkLen);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    sss_host_digest_context_free(&md);
    masterSkLen = 16;
    LOG_MAU8_D("Master Secret", masterSk, masterSkLen);
    /*Set the Master secret as AES Key*/
    status = sss_host_key_store_set_key(
        pStatic_ctx->masterSec.keyStore, &pStatic_ctx->masterSec, masterSk, masterSkLen, masterSkLen * 8, NULL, 0);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
cleanup:
    return status;
}

static void set_secp256r1nist_header(uint8_t *pbKey, size_t *pbKeyByteLen)
{
    unsigned int i = 0;
    /* clang-format off */
    uint8_t temp[112] = { 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
        0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01,
        0x07, 0x03, 0x42, 0x00 };
    /* clang-format on */

    for (i = 0; i < *pbKeyByteLen; i++) {
        temp[26 + i] = pbKey[i];
    }

    if ((UINT_MAX - (*pbKeyByteLen)) < 26) {
        return;
    }
    *pbKeyByteLen = *pbKeyByteLen + 26;
    memcpy(pbKey, temp, *pbKeyByteLen);
}

sss_status_t nxECKey_InternalAuthenticate(pSe05xSession_t se05xSession,
    SE05x_AuthCtx_ECKey_t *pAuthFScp,
    uint8_t *hostEckaPubKey,
    size_t hostEckaPubKeyLen,
    uint8_t *rndData,
    size_t *rndDataLen,
    uint8_t *receipt,
    size_t *receiptLen)
{
    sss_status_t status  = kStatus_SSS_Fail;
    smStatus_t retStatus = SM_NOT_OK;
    int tlvRet           = 0;
    uint8_t cmdbuf[256];
    size_t cmdbufLen    = 0;
    uint8_t *pCmdbuf    = NULL;
    uint8_t rspbuf[256] = {0};
    uint8_t *pRspbuf    = &rspbuf[0];
    size_t rspbufLen    = ARRAY_SIZE(rspbuf);
    size_t rspIndex     = 0;
    sss_digest_t md;
    uint8_t md_host5F37[32];
    size_t md_host5F37Len              = sizeof(md_host5F37);
    NXECKey03_StaticCtx_t *pStatic_ctx = pAuthFScp->pStatic_ctx;

    const uint8_t tagEpkSeEcka[] = {0x7F, 0x49};
    const uint8_t tagSigSeEcka[] = {0x5F, 0x37};

    const tlvHeader_t hdr               = {{CLA_GP_7816 | CLA_GP_SECURITY_BIT, INS_GP_INTERNAL_AUTHENTICATE, 00, 00}};
    uint8_t scpParms[3]                 = {0xAB, SCP_CONFIG, SECURITY_LEVEL};
    uint8_t appletName[APPLET_NAME_LEN] = APPLET_NAME;
    sss_asymmetric_t asym;
    uint8_t sig_host5F37[100] = {0};
    size_t sig_host5F37Len    = sizeof(sig_host5F37);

    size_t cntrlRefTemp_Len = 0 + 1 + 1 + APPLET_NAME_LEN /*TLV AID */ + 1 + 1 + sizeof(scpParms) /* TLV SCP Params */ +
                              1 + 1 + 1 /* TLV Keytype */ + 1 + 1 + 1 /* TLV KeyLEN */;

#if NX_LOG_ENABLE_SCP_DEBUG
    nLog("APDU", NX_LEVEL_DEBUG, "ECKey Internal authenticate []");
#endif /* VERBOSE_APDU_LOGS */
    cmdbuf[0] = kSE05x_TAG_GP_CONTRL_REF_PARM; // Tag Control reference template
    cmdbuf[1] = (uint8_t)cntrlRefTemp_Len;
    cmdbufLen = 2;
    pCmdbuf   = &cmdbuf[2];
    tlvRet    = TLVSET_u8buf("SE05x AID", &pCmdbuf, &cmdbufLen, kSE05x_GP_TAG_AID, appletName, APPLET_NAME_LEN);
    ENSURE_OR_GO_CLEANUP(tlvRet == 0);
    tlvRet = TLVSET_u8buf("SCP parameters", &pCmdbuf, &cmdbufLen, kSE05x_GP_TAG_SCP_PARMS, scpParms, sizeof(scpParms));
    ENSURE_OR_GO_CLEANUP(tlvRet == 0);
    tlvRet = TLVSET_U8("Key Type", &pCmdbuf, &cmdbufLen, kSE05x_GP_TAG_KEY_TYPE, GPCS_KEY_TYPE_AES);
    ENSURE_OR_GO_CLEANUP(tlvRet == 0);
    tlvRet = TLVSET_U8("Key length", &pCmdbuf, &cmdbufLen, kSE05x_GP_TAG_KEY_LEN, GPCS_KEY_LEN_AES);
    ENSURE_OR_GO_CLEANUP(tlvRet == 0);

    /*Put the ephemral host ECKA pub key */
    *pCmdbuf++ = tagEpkSeEcka[0]; //Tag is 2 byte */
    cmdbufLen++;
    *pCmdbuf++ = tagEpkSeEcka[1];
    cmdbufLen++;
    *pCmdbuf++ = (uint8_t)hostEckaPubKeyLen;
    cmdbufLen++;
    memcpy(pCmdbuf, hostEckaPubKey, hostEckaPubKeyLen);
    if ((UINT_MAX - cmdbufLen) < hostEckaPubKeyLen) {
        goto cleanup;
    }
    cmdbufLen += hostEckaPubKeyLen;

    /* Get the sha256 hash of Control_refernce_template + host ECKA Pub key */
    status = sss_host_digest_context_init(
        &md, pStatic_ctx->HostEcdsaObj.keyStore->session, kAlgorithm_SSS_SHA256, kMode_SSS_Digest);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    status = sss_host_digest_one_go(&md, cmdbuf, cmdbufLen, md_host5F37, &md_host5F37Len);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    sss_host_digest_context_free(&md);

    /* Get the signiture over hash*/
    status = sss_host_asymmetric_context_init(&asym,
        pStatic_ctx->HostEcdsaObj.keyStore->session,
        &pStatic_ctx->HostEcdsaObj,
        kAlgorithm_SSS_SHA256,
        kMode_SSS_Sign);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    status = sss_host_asymmetric_sign_digest(&asym, md_host5F37, md_host5F37Len, sig_host5F37, &sig_host5F37Len);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    sss_host_asymmetric_context_free(&asym);

    /* Put the Control refernce template Value signature*/
    if (cmdbufLen > sizeof(cmdbuf) - 3 - sig_host5F37Len) {
        status = kStatus_SSS_Fail;
        goto cleanup;
    }
    pCmdbuf    = &cmdbuf[cmdbufLen];
    *pCmdbuf++ = tagSigSeEcka[0];
    cmdbufLen++;
    *pCmdbuf++ = tagSigSeEcka[1];
    cmdbufLen++;
    *pCmdbuf++ = (uint8_t)sig_host5F37Len;
    cmdbufLen++;
    memcpy(pCmdbuf, sig_host5F37, sig_host5F37Len);
    if ((UINT_MAX - cmdbufLen) < sig_host5F37Len) {
        status = kStatus_SSS_Fail;
        goto cleanup;
    }
    cmdbufLen += sig_host5F37Len;
    status    = kStatus_SSS_Fail;
    retStatus = DoAPDUTxRx_s_Case4(se05xSession, &hdr, cmdbuf, cmdbufLen, rspbuf, &rspbufLen);
    ENSURE_OR_GO_CLEANUP(retStatus == SM_OK);
    tlvRet =
        tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_GP_TAG_DR_SE, rndData, rndDataLen); /* Get the Random No */
    ENSURE_OR_GO_CLEANUP(tlvRet == 0);
    tlvRet =
        tlvGet_u8buf(pRspbuf, &rspIndex, rspbufLen, kSE05x_GP_TAG_RECEIPT, receipt, receiptLen); /* Get the Receipt */
    ENSURE_OR_GO_CLEANUP(tlvRet == 0);
    ENSURE_OR_GO_CLEANUP((rspIndex + 2) == rspbufLen);
    retStatus = (smStatus_t)((pRspbuf[rspIndex] << 8) | (pRspbuf[rspIndex + 1]));
    ENSURE_OR_GO_CLEANUP(retStatus == SM_OK);
    status = kStatus_SSS_Success;

cleanup:
    return status;
}

int get_u8buf_2bTag(uint8_t *buf, size_t *pBufIndex, const size_t bufLen, uint16_t tag, uint8_t *rsp, size_t *pRspLen)
{
    int retVal       = 1;
    uint8_t *pBuf    = buf + (*pBufIndex);
    uint16_t got_tag = 0;
    size_t extendedLen;
    size_t rspLen;

    if (bufLen < 3) {
        goto cleanup;
    }
    if ((*pBufIndex) > bufLen - 3 /* 2 byte Tag + len */) {
        goto cleanup;
    }

    got_tag = ((*pBuf++) << 8) & 0xFFFF;
    got_tag |= ((*pBuf++)) & 0xFFFF;

    if (got_tag != tag) {
        goto cleanup;
    }
    rspLen = *pBuf++;

    if (rspLen <= 0x7FU) {
        extendedLen = rspLen;
        *pBufIndex += (2 + 1);
    }
    else if (rspLen == 0x81) {
        if ((*pBufIndex) > bufLen - 1 /* Ext len */) {
            goto cleanup;
        }
        extendedLen = *pBuf++;
        *pBufIndex += (2 + 1 + 1);
    }
    else if (rspLen == 0x82) {
        if ((*pBufIndex) > bufLen - 2 /* Ext len */) {
            goto cleanup;
        }
        extendedLen = *pBuf++;
        extendedLen = (extendedLen << 8) | *pBuf++;
        *pBufIndex += (2 + 1 + 2);
    }
    else {
        goto cleanup;
    }

    if (extendedLen > *pRspLen) {
        goto cleanup;
    }
    if (extendedLen > (bufLen - *pBufIndex)) {
        goto cleanup;
    }

    *pRspLen = extendedLen;
    *pBufIndex += extendedLen;
    while (extendedLen-- > 0) {
        *rsp++ = *pBuf++;
    }
    retVal = 0;
cleanup:
    return retVal;
}

sss_status_t nxECKey_Calculate_Shared_secret(
    SE05x_AuthCtx_ECKey_t *pAuthFScp, uint8_t *sharedSecret, size_t *sharedSecretLen)
{
    sss_status_t status = kStatus_SSS_Fail;
    sss_derive_key_t dervCtx;
    sss_object_t shsSecret = {0};

    NXECKey03_StaticCtx_t *pStatic_ctx = pAuthFScp->pStatic_ctx;
    size_t sharedSecBitLen             = 0;

    status = sss_host_key_object_init(&shsSecret, pStatic_ctx->SeEcPubKey.keyStore);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = sss_host_key_object_allocate_handle(
        &shsSecret, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, 32, kKeyObject_Mode_Transient);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = sss_host_derive_key_context_init(&dervCtx,
        pStatic_ctx->HostEcKeypair.keyStore->session,
        &pStatic_ctx->HostEcKeypair,
        kAlgorithm_SSS_ECDH,
        kMode_SSS_ComputeSharedSecret);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = sss_host_derive_key_dh(&dervCtx, &pStatic_ctx->SeEcPubKey, &shsSecret);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status =
        sss_host_key_store_get_key(&shsSecret.keyStore, &shsSecret, sharedSecret, sharedSecretLen, &sharedSecBitLen);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    LOG_MAU8_D("Shared Secret", sharedSecret, *sharedSecretLen);

cleanup:
    sss_host_derive_key_context_free(&dervCtx);
    sss_host_key_object_free(&shsSecret);

    return status;
}

#endif /* defined SSS_HAVE_SCP_SCP03_SSS */
#endif /* SSS_HAVE_APPLET_SE05X_IOT */
