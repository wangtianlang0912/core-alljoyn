/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include "gtest/gtest.h"
#include "Common.h"
#include "SecurityManagerFactory.h"
#include "PermissionMgmt.h"
#include "TestUtil.h"
#include <semaphore.h>
#include <stdio.h>

/**
 * Several nominal tests for membership certificates.
 */
namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;
using namespace std;

class MembershipNominalTests :
    public ClaimedTest {
  private:

  protected:

  public:

    void SetUp()
    {
        ClaimedTest::SetUp();

        //Dummy manifest
        qcc::String ifn = "org.allseen.control.TV";
        qcc::String mbr = "*";
        Type t = Type::SIGNAL;
        Action a = Action::PROVIDE;
        appInfo.manifest.AddRule(ifn, mbr, t, a);
    }
};

/**
 * \test The test should verify that after a device is claimed:
 *       -# Membership certificates can be installed on it.
 *       -# Membership certificates can be removed.
 * */
TEST_F(MembershipNominalTests, SuccessfulMembership)
{
    /* Test Memberships ...*/
    GuildInfo guildInfo1;
    guildInfo1.guid = GUID128("B509480EE75397473B5A000B82A7E37E");
    guildInfo1.name = "MyGuild 1";
    guildInfo1.desc = "My test guild 1 description";

    GuildInfo guildInfo2;
    guildInfo2.guid = GUID128("E4DD81F54E7DB918EA5B2CE79D72200E");
    guildInfo2.name = "MyGuild 2";
    guildInfo2.desc = "My test guild 2 description";

    ASSERT_EQ(stub->GetMembershipCertificates().size(), ((size_t)0));
    ASSERT_EQ(secMgr->StoreGuild(guildInfo1, false), ER_OK);
    ASSERT_EQ(secMgr->InstallMembership(appInfo, guildInfo1), ER_OK);
    std::map<GUID128, qcc::String> certificates = stub->GetMembershipCertificates();
    ASSERT_EQ(certificates.size(), ((size_t)1));
    std::map<GUID128, qcc::String>::iterator it = certificates.find(guildInfo1.guid);
    ASSERT_FALSE(it == certificates.end());
    ASSERT_EQ(it->first, guildInfo1.guid);

    ASSERT_EQ(secMgr->StoreGuild(guildInfo2, false), ER_OK);
    ASSERT_EQ(secMgr->InstallMembership(appInfo, guildInfo2), ER_OK);
    certificates = stub->GetMembershipCertificates();
    ASSERT_EQ(certificates.size(), ((size_t)2));
    it = certificates.find(guildInfo2.guid);
    ASSERT_FALSE(it == certificates.end());
    ASSERT_EQ(it->first, guildInfo2.guid);

    ASSERT_EQ(secMgr->RemoveMembership(appInfo, guildInfo1), ER_OK);
    certificates = stub->GetMembershipCertificates();
    ASSERT_EQ(certificates.size(), ((size_t)1));
    it = certificates.find(guildInfo1.guid);
    ASSERT_TRUE(it == certificates.end());

    ASSERT_EQ(secMgr->RemoveMembership(appInfo, guildInfo2), ER_OK);
    certificates = stub->GetMembershipCertificates();
    ASSERT_EQ(certificates.size(), ((size_t)0));
    it = certificates.find(guildInfo2.guid);
    ASSERT_TRUE(it == certificates.end());
}

/**
 * \test The test should verify that InstallMembership and RemoveMembership with invalid arguments is handled in a robust way
 * */
TEST_F(MembershipNominalTests, InvalidArgsMembership)
{
    //Stub is claimed ...
    GuildInfo guildInfo;
    guildInfo.name = "MyGuild";
    guildInfo.desc = "My test guild description";

    //Guild is not known to security manager.
    ASSERT_EQ(ER_FAIL, secMgr->InstallMembership(appInfo, guildInfo));
    ASSERT_EQ(ER_FAIL, secMgr->RemoveMembership(appInfo, guildInfo));

    //Guild known, invalid app.
    ASSERT_EQ(secMgr->StoreGuild(guildInfo, false), ER_OK);
    ApplicationInfo invalid = appInfo;
    invalid.publicKey = PublicKey();
    ASSERT_EQ(ER_FAIL, secMgr->InstallMembership(invalid, guildInfo));
    ASSERT_EQ(ER_FAIL, secMgr->RemoveMembership(invalid, guildInfo));

    ASSERT_EQ(stub->GetMembershipCertificates().size(), ((size_t)0));
    ASSERT_EQ(ER_OK, secMgr->InstallMembership(appInfo, guildInfo));
    ASSERT_EQ(ER_OK, secMgr->InstallMembership(appInfo, guildInfo));
    ASSERT_EQ(ER_OK, secMgr->InstallMembership(appInfo, guildInfo));
    ASSERT_EQ(stub->GetMembershipCertificates().size(), ((size_t)1));
    ASSERT_EQ(ER_OK, secMgr->RemoveMembership(appInfo, guildInfo));
    ASSERT_EQ(stub->GetMembershipCertificates().size(), ((size_t)0));
    ASSERT_EQ(ER_FAIL, secMgr->RemoveMembership(appInfo, guildInfo));

    invalid = appInfo;
    invalid.busName = "invalidBusname";
    ASSERT_EQ(ER_OK, secMgr->InstallMembership(invalid, guildInfo));
    ASSERT_EQ(ER_OK, secMgr->InstallMembership(invalid, guildInfo));
    ASSERT_EQ(stub->GetMembershipCertificates().size(), ((size_t)1));
    ASSERT_EQ(ER_OK, secMgr->RemoveMembership(invalid, guildInfo));
    ASSERT_EQ(stub->GetMembershipCertificates().size(), ((size_t)0));
    ASSERT_EQ(ER_FAIL, secMgr->RemoveMembership(invalid, guildInfo));

    ASSERT_EQ(ER_OK, secMgr->InstallMembership(appInfo, guildInfo));

    GuildInfo guildInfo2;
    guildInfo2.name = "2 MyGuild";
    guildInfo2.desc = "2 My test guild description";
    secMgr->StoreGuild(guildInfo2, true);

    destroy();
    ASSERT_NE(ER_OK, secMgr->InstallMembership(appInfo, guildInfo));
    ASSERT_NE(ER_OK, secMgr->RemoveMembership(appInfo, guildInfo2));
}
}
//namespace
