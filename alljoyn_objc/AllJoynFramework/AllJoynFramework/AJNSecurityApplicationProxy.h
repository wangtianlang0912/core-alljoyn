////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNPermissionConfigurator.h"
#import "AJNBusAttachment.h"
#import "AJNGUID.h"
#import "AJNCryptoECC.h"
#import "AJNCertificateX509.h"
#import "AJNSessionOptions.h"

@interface AJNSecurityApplicationProxy : AJNObject

/**
 * SecurityApplicationProxy Constructor
 */
- (id)initWithBus:(AJNBusAttachment*)bus withBusName:(NSString*)busName inSession:(AJNSessionId)sessionId;

/**
 * Returns the XML version of the manifest template. The returned value
 * is managed by the caller and has to be freed using DestroyManifestTemplate.
 *
 * @param[out] manifestTemplateXml  The manifest template in XML format.
 *
 * @return
 *  - #ER_OK if successful.
 *  - An error status indicating failure.
 */
- (QStatus)getManifestTemplateAsXml:(NSString**)manifestTemplateXml;

/**
 * An enumeration representing the current state of
 * the application.  The list of valid values:
 *
 * | Value         | Description                                                      |
 * |---------------|------------------------------------------------------------------|
 * | NOT_CLAIMABLE | The application is not claimed and not accepting claim requests. |
 * | CLAIMABLE     | The application is not claimed and is accepting claim requests.  |
 * | CLAIMED       | The application is claimed and can be configured.                |
 * | NEED_UPDATE   | The application is claimed, but requires a configuration update. |
 *
 * @param[out] applicationState Current state of the service.
 *
 * @return
 *  - #ER_OK if successful
 *  - an error status indicating failure
 */
- (QStatus)getApplicationState:(AJNApplicationState*)applicationState;

/**
 * The Elliptic Curve Cryptography public key used by the application's keystore
 * to identify itself. The public key persists across any
 * ManagedApplication.Reset() call.  However, if the keystore is cleared via the
 * BusAttachment::ClearKeyStore() or using Config.FactoryReset(), the public key
 * will be regenerated.
 *
 * @param[out] eccPublicKey The Elliptic Curve Cryptography public key
 *
 * @return
 *  - #ER_OK if successful
 *  - an error status indicating failure
 */
- (QStatus)getECCPublicKey:(AJNECCPublicKey**)eccPublicKey;

/**
 * The authentication mechanisms the application supports for the claim process.
 * It is a bit mask.
 *
 * | Mask                  | Description              |
 * |-----------------------|--------------------------|
 * | CAPABLE_ECDHE_NULL    | claiming via ECDHE_NULL  |
 * | CAPABLE_ECDHE_PSK     | claiming via ECDHE_PSK   |
 * | CAPABLE_ECDHE_SPEKE   | claiming via ECDHE_SPEKE |
 * | CAPABLE_ECDHE_ECDSA   | claiming via ECDHE_ECDSA |
 *
 * @param[out] claimCapabilities The authentication mechanisms the application supports
 *
 * @return
 *  - #ER_OK if successful
 *  - an error status indicating failure
 */
- (QStatus)getClaimCapabilites:(AJNClaimCapabilities*)claimCapabilities;


/**
 * The additional information information on the claim capabilities.
 * It is a bit mask.
 *
 * | Mask                              | Description                                   |
 * |-----------------------------------|-----------------------------------------------|
 * | PSK_GENERATED_BY_SECURITY_MANAGER | PSK or password generated by Security Manager |
 * | PSK_GENERATED_BY_APPLICATION      | PSK or password generated by application      |
 *
 * @param[out] claimCapabilitiesAdditionalInfo The additional information
 *
 * @return
 *  - #ER_OK if successful
 *  - an error status indicating failure
 */
- (QStatus)getClaimCapabilityAdditionalInfo:(AJNClaimCapabilityAdditionalInfo*)claimCapabilitiesAdditionalInfo;

/**
 * Claim the app using a manifests in XML format. This will make
 * the claimer the admin and certificate authority. The KeyInfo
 * object description is shown below.
 *
 * Access restriction: None if the app is not yet claimed. An error will be
 * raised if the app has already been claimed.
 *
 * @param[in]   certificateAuthority    A KeyInfo object representing the
 *                                      public key of the certificate authority.
 * @param[in]   adminGroupId            The admin group Id.
 * @param[in]   adminGroup              A KeyInfo object representing the admin
 *                                      security group authority.
 * @param[in]   identityCertChain       The identity certificate chain for the
 *                                      claimed app.  The leaf cert is listed first
 *                                      followed by each intermediate Certificate
 *                                      Authority's certificate, ending in the trusted
 *                                      root's certificate.
 * @param[in]   manifestsXmls           An array of NSString containing
 *                                      the application's manifests in XML format.
 * @return
 *  - #ER_OK if successful
 *  - #ER_PERMISSION_DENIED Error raised when the application is not claimable
 *  - #ER_INVALID_CERTIFICATE Error raised when the identity certificate
 *                            chain is not valid
 *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage
 *                                  is not AllJoyn specific.
 *  - #ER_DIGEST_MISMATCH Error raised when none of the provided signed manifests are
 *                                         valid for the given identity certificate.
 *  - an error status indicating failure
 */
- (QStatus)claim:(AJNKeyInfoNISTP256*)certificateAuthority adminGroupId:(AJNGUID128*)adminGroupId adminGroup:(AJNKeyInfoNISTP256*)adminGroup identityCertChain:(NSArray*)identityCertChain manifestsXmls:(NSArray*)manifestsXmls;

/**
 * This method allows an admin to update the application's identity certificate
 * chain and its manifests using the Manifest objects.
 *
 * After having a new identity certificate installed, the target bus clears
 * out all of its peer's secret and session keys, so the next call will get
 * security violation. After calling UpdateIdentity, SecureConnection(true)
 * should be called to force the peers to create a new set of secret and
 * session keys.
 *
 * It is highly recommended that element 0 of identityCertificateChain, the peer's
 * end entity certificate, be of type qcc::IdentityCertificate. Other certs can be
 * of this or the base CertificateX509 type.
 *
 * The target peer also clears all manifests it has already stored, and so all
 * manifests the peer needs must be sent again. Use GetManifests to retrieve
 * the currently-installed manifests before calling UpdateIdentity to reuse them.
 *
 * Manifests must already be signed by the authority that issued the identity
 * certificate chain.
 *
 * @see ProxyBusObject.SecureConnection(bool)
 * @see SecurityApplicationProxy.GetManifests(std::vector<Manifest>&)
 *
 * @param[in] identityCertificateChain             the identity certificate
 * @param[in] manifestsXmls                        an array of NSStrings containing the
 *                                                 signed manifests to install on the
 *                                                 application in XmlFormat
 *
 * @return
 *  - #ER_OK if successful
 *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
 *  - #ER_INVALID_CERTIFICATE Error raised when the identity certificate chain is not valid
 *  - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage is not AllJoyn specific
 *  - #ER_DIGEST_MISMATCH Error raised when the digest is not accepted
 *  - an error status indicating failure
 */
- (QStatus)updateIdentity:(NSArray*)identityCertificateChain manifestsXmls:(NSArray*)manifestsXmls;

/**
 * This method allows an admin to install the permission policy to the
 * application using an XML version of the policy.
 * Any existing policy will be replaced if the new policy version
 * number is greater than the existing policy's version number.
 *
 * After having a new policy installed, the target bus clears out all of
 * its peer's secret and session keys, so the next call will get security
 * violation. After calling UpdatePolicy, SecureConnection(true) should be
 * called to force the peers to create a new set of secret and session keys.
 *
 * Until ASACORE-2755 is fixed the caller must include all default policies
 * (containing information about the trust anchors) with each call, so they
 * would not be removed.
 *
 * @see ProxyBusObject.SecureConnection(bool)
 *
 * @param[in] policy    The new policy in XML format. For the policy XSD refer to
 *                      alljoyn_core/docs/policy.xsd.
 *
 * @return
 *  - #ER_OK if successful.
 *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission.
 *  - #ER_POLICY_NOT_NEWER  Error raised when the new policy does not have a
 *                          greater version number than the existing policy.
 *  - #ER_XML_MALFORMED     If the policy was not in the valid XML format.
 *  - an error status indicating failure.
 */
- (QStatus)updatePolicyFromXml:(NSString*)policy;

/**
 * This method allows the admin to install a membership cert chain to the
 * application.
 *
 * It is highly recommended that element 0 of certificateChain, the peer's
 * end entity certificate, be of type qcc::MembershipCertificate, so that the correct
 * Extended Key Usage (EKU) is set. The remaining certificates in the chain can be
 * of this or the base CertificateX509 type.
 *
 * @param[in] certificateChain      An array of AJNCertificateX509 containing the
 *                                  membership certificate chain. It can be a
 *                                  single certificate if it is issued by the security
 *                                  group authority.
 *
 * @return
 *  - #ER_OK if successful
 *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
 *  - #ER_DUPLICATE_CERTIFICATE Error raised when the membership certificate
 *                              is already installed.
 *  - #ER_INVALID_CERTIFICATE Error raised when the membership certificate is not valid.
 *  - an error status indicating failure
 */
- (QStatus)installMembership:(NSArray*)certificateChain;

/**
 * This method allows an admin to reset the application to its original state
 * prior to claim.  The application's security 2.0 related configuration is
 * discarded.  The application is no longer claimed.
 *
 * If the keystore is cleared by the BusAttachment::ClearKeyStore() call, this
 * Reset() method call is not required.  The Configuration service's
 * FactoryReset() call in fact clears the keystore, so this Reset() call is not
 * required in that scenario.
 *
 * @return
 *  - #ER_OK if successful
 *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
 *  - an error status indicating failure
 */
- (QStatus)reset;

/**
 * This method allows an admin to remove the currently installed policy.  The
 * application reverts back to the default policy generated during the claiming
 * process.
 *
 * @return
 *  - #ER_OK if successful
 *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
 *  - an error status indicating failure
 */
- (QStatus)resetPolicy;

/**
 * This method notifies the application about the fact that the Security Manager
 * will start to make changes to the application's security settings.
 *
 * @return
 *  - #ER_OK if successful
 *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
 *  - some other error status indicating failure
 */
- (QStatus)startManagement;

/**
 * This method notifies the application about the fact that the Security Manager
 * has finished making changes to the application's security settings.
 *
 * @return
 *  - #ER_OK if successful
 *  - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
 *  - some other error status indicating failure
 */
- (QStatus)endManagement;

/**
 * Adds an identity certificate thumbprint to and signs the manifest XML.
 *
 * @param[in]    identityCertificate The identity certificate of the remote application that will
 *                                   use the signed manifest.
 * @param[in]    privateKey          The signing key. It must be the same one used to sign the
 *                                   identity certificate.
 * @param[in]    unsignedManifestXml The unsigned manifest in XML format. The XML schema can be found
 *                                   under alljoyn_core/docs/manifest_template.xsd.
 * @param[out]   signedManifestXml   The signed manifest in XML format.
 *                                   The string is managed by the caller and must be later destroyed
 *                                   using destroySignedManifest.
 * @return
 *  - #ER_OK if successful
 *  - an error status indicating failure
 */
+ (QStatus)signManifest:(AJNCertificateX509*)identityCertificate privateKey:(AJNECCPrivateKey*)privateKey unsignedManifestXml:(NSString*)unsignedManifestXml signedManifestXml:(char**)signedManifestXml;

/**
 * Destroys the signed manifest XML created by SignManifest.
 *
 * @param[in]    signedManifestXml  The signed manifest in XML format.
 */
+ (void)destroySignedManifest:(char*)signedManifestXml;

/**
 * Adds an identity certificate thumbprint and retrieves the digest of the manifest XML for signing.
 *
 * @param[in]    unsignedManifestXml    The unsigned manifest in XML format. The XML schema can be found
 *                                      under alljoyn_core/docs/manifest_template.xsd.
 * @param[in]    identityCertificate    The identity certificate of the remote application that will use
 *                                      the signed manifest.
 * @param[out]   digest                 Pointer to receive the byte array containing the digest to be
 *                                      signed with ECDSA-SHA256. This array is managed by the caller and must
 *                                      be later destroyed using DestroyManifestDigest.
 * @param[out]   digestSize             size_t to receive the size of the digest array.
 *
 * @return
 *          - #ER_OK            If successful.
 *          - #ER_XML_MALFORMED If the unsigned manifest is not compliant with the required format.
 *          - Other error status indicating failure.
 */
+ (QStatus)computeManifestDigest:(NSString*)unsignedManifestXml identityCertificate:(AJNCertificateX509*)identityCertificate digest:(uint8_t**)digest;

/**
 * Destroys a digest buffer returned by a call to computeManifestDigest.
 *
 * @param[in]   digest  Digest array returned by computeManifestDigest.
 */
+ (void)destroyManifestDigest:(uint8_t*)digest;

@end
