/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "TestUtil.h"

using namespace ajn::securitymgr;

/** @file ClaimingTests.cc */

namespace secmgr_tests {
class AutoRejector :
    public ManifestListener {
    bool ApproveManifest(const OnlineApplication& app,
                         const Manifest& manifest)
    {
        QCC_UNUSED(app);
        QCC_UNUSED(manifest);

        return false;
    }
};

class RejectAfterAcceptListener :
    public ManifestListener {
  public:
    RejectAfterAcceptListener(const shared_ptr<SecurityAgent>& _secMgr) : secMgr(_secMgr)
    {
    }

    bool ApproveManifest(const OnlineApplication& app,
                         const Manifest& manifest)
    {
        QCC_UNUSED(app);
        QCC_UNUSED(manifest);

        secMgr->SetManifestListener(&ar);

        return true;
    }

  private:
    AutoRejector ar;
    shared_ptr<SecurityAgent> secMgr;
};

class ClaimingTests :
    public BasicTest {
};

/**
 * @test Claim an application and check that it becomes CLAIMED.
 *       -# Start the application.
 *       -# Make sure the application is in a CLAIMABLE state.
 *       -# Claim the application.
 *       -# Accept the manifest of the application.
 *       -# Check whether the application becomes CLAIMED.
 **/
TEST_F(ClaimingTests, SuccessfulClaim) {
    /* Check that the app is not there yet */
    ASSERT_EQ(ER_END_OF_DATA, secMgr->GetApplication(lastAppInfo));

    /* Start the application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    /* Create identity */
    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->GetManagedApplication(lastAppInfo));

    /* Try to claim again */
    ASSERT_NE(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
}

/**
 * @test Reject the manifest during claiming and check whether the application
 *       becomes CLAIMABLE again.
 *       -# Claim the remote application.
 *       -# Reject the manifest.
 *       -# Check whether the agent returns an ER_MANIFEST_REJECTED error.
 *       -# Check whether the application remains CLAIMABLE.
 **/

TEST_F(ClaimingTests, RejectManifest) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    AutoRejector ar;
    secMgr->SetManifestListener(&ar);

    ASSERT_EQ(ER_MANIFEST_REJECTED, secMgr->Claim(lastAppInfo, idInfo));
    secMgr->SetManifestListener(nullptr);
}

/**
 * @test Basic robustness tests for the claiming, including input validation,
 *       unavailability of manifest listener/CA.
 *      -# Claiming an off-line/unknown application should fail.
 *      -# Claiming using an unknown identity should fail.
 *      -# Claiming an application that is NOT_CLAIMABALE should fail.
 *      -# Claiming an application that is CLAIMED should fail.
 *      -# Claiming an application that NEED_UPDATE should fail.
 *
 *      -# Claiming when no ManifestListener is set should fail.
 *      -# Claiming when application did not specify any manifest should fail.
 */
TEST_F(ClaimingTests, BasicRobustness) {
    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "StoredTestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);
    ASSERT_EQ(ER_FAIL, secMgr->Claim(lastAppInfo, idInfo));     // No test app exists (or offline)

    TestApplication testApp;
    IdentityInfo inexistentIdInfo;
    inexistentIdInfo.guid = GUID128();
    inexistentIdInfo.name = "InexistentTestIdentity";
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    ASSERT_EQ(ER_FAIL, secMgr->Claim(lastAppInfo, inexistentIdInfo)); // Claim a claimable app with inexistent ID

    testApp.SetApplicationState(PermissionConfigurator::NOT_CLAIMABLE);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::NOT_CLAIMABLE, true));
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_CLAIM, ER_PERMISSION_DENIED));

    testApp.SetApplicationState(PermissionConfigurator::CLAIMED);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_CLAIM, ER_PERMISSION_DENIED));

    testApp.SetApplicationState(PermissionConfigurator::NEED_UPDATE);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::NEED_UPDATE, true));
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_CLAIM, ER_PERMISSION_DENIED));

    secMgr->SetManifestListener(nullptr);
    testApp.SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    ASSERT_EQ(ER_FAIL, secMgr->Claim(lastAppInfo, idInfo)); // Secmgr has no mft listener

    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, false));
    testApp.Reset();

    TestApplication testApp2("Test2");
    testApp2.Start(false); // No default manifest
    AutoAccepter aa;
    secMgr->SetManifestListener(&aa);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, secMgr->Claim(lastAppInfo, idInfo));  // app has no manifest
}

/**
 * @test Basic robustness test with a faulty CA.
 *       //TODO Wouldn't AS-1616 suffice?
 *      -# Claiming when the CA does not return its public key should fail.
 *      -# Claiming when the CA does not return an identity certificate should
 *         fail.
 *      -# Claiming when the CA does not persisted claim result should succeed,
 *         as to line up with the state persisted in the database.
 *      -# Restarting the application should not be reset.
 */
TEST_F(ClaimingTests, DISABLED_BasicRobustnessCAToAgent) {
}
/**
 * @test Recovery from failure of notifying the CA of failure of claiming an
 *       application should be graceful.
 *       -# Claim a CLAIMABLE application with a known identity.
 *       -# The manifest is approved.
 *       -# Notifying the CA that claiming will start succeeds.
 *       -# The Claim call to the application fails.
 *       -# Notifying the CA of this failure also fails.
 *       -# Stop the application.
 *       -# Restore the connection to the CA.
 *       -# Start the application.
 *       -# The application should be claimed automatically.
 */
TEST_F(ClaimingTests, DISABLED_RecoveryFromFinishClaimingFailure) {
}

/**
 * @test Changing the manifest listener when being in the callback of the
 *       original manifest listener should work.
 *       -# Claim a CLAIMABLE application with a known identity.
 *       -# While the manifest listener is called to approve the manifest, a
 *          new manifest listener is installed to reject the manifest.
 *       -# The original listener accepts the manifest.
 *       -# The application should be claimed.
 *       -# Start a new application and try claiming it.
 *       -# The manifest should be rejected and the claiming should fail.
 *       -# Make sure the new application is still claimable.
 */
TEST_F(ClaimingTests, ConcurrentManifestListenerUpdate) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    RejectAfterAcceptListener rejectAfterAccept(secMgr);
    secMgr->SetManifestListener(&rejectAfterAccept);

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));

    TestApplication testApp2("NewTestApp");
    ASSERT_EQ(ER_OK, testApp2.Start());

    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    ASSERT_EQ(ER_MANIFEST_REJECTED, secMgr->Claim(lastAppInfo, idInfo));

    testApp2.SetApplicationState(PermissionConfigurator::CLAIMABLE); // Trigger another event
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
}

/**
 * @test Verify claiming with Out-Of-Band (OOB) succeeds.
 *       -# Start an application and make sure it's in the CLAIMABLE state
 *          with PSK preference; i.e., OOB.
 *       -# Make sure that the application has generated the PSK.
 *       -# Verify the the security agent uses the same PSK for OOB claiming
 *          and accepts the manifest.
 *       -# Verify that the application is CLAIMED and online.
 *       -# Reset/remove the application and make sure it's claimable again and
 *          repeat the scenario but make sure that the PSK in generated by the
 *          security agent and that the application uses that same one.
 *       -# Verify that claiming was successful and that the application is in
 *          CLAIMED state and online.
 *
 */
TEST_F(ClaimingTests, DISABLED_OOBSuccessfulClaiming) {
}

/**
 * @test Verify claiming with Out-Of-Band (OOB) fails when wrong PSK is used.
 *       -# Start an application and make sure it's in the CLAIMABLE state
 *          with PSK preference; i.e., OOB.
 *       -# Make sure that the security agent has generated the PSK.
 *       -# Verify the the application uses a different PSK for OOB claiming.
 *       -# Verify that the application is still CLAIMABLE and online and that
 *          claiming has failed.
 *       -# Repeat the scenario where the PSK is generated by the application
 *          instead of the security agent and make sure PSK claiming fails.
 */
TEST_F(ClaimingTests, DISABLED_OOBFailedClaiming) {
}

/**
 * @test Verify claiming with Out-Of-Band (OOB) times-out.
 *       -# Start an application and make sure it's in the CLAIMABLE state
 *          with PSK preference; i.e., OOB.
 *       -# Try to claim the application but do not provide a PSK and wait
 *          for the OOB default/predefined timeout period.
 *       -# Verify that claiming has timed-out.
 *       -# Verify that the application is CLAIMABLE and online.
 */
TEST_F(ClaimingTests, DISABLED_OOBClaimingTimeout) {
}
} // namespace
