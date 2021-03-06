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
#include <vector>
#include <gtest/gtest.h>

#include <alljoyn/Status.h>
#include <qcc/XmlElement.h>
#include <qcc/String.h>
#include <qcc/StringSource.h>

using namespace qcc;

#define VALID_ROOT_XML "<root/>"
#define VALID_CHILD_XML "<child/>"

class XmlElementTest : public testing::Test {
  public:
    XmlElementTest() :
        parent(nullptr),
        child(nullptr),
        root(nullptr)
    { };

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlElement::GetRoot(VALID_ROOT_XML, &parent));
        ASSERT_EQ(ER_OK, XmlElement::GetRoot(VALID_CHILD_XML, &child));
        ASSERT_EQ(ER_OK, XmlElement::GetRoot(VALID_ROOT_XML, &root));
    }

    virtual void TearDown()
    {
        delete parent;
        delete root;
    }

  protected:
    XmlElement* parent;
    XmlElement* child;
    XmlElement* root;
};

TEST(XmlElement, ShouldFailGetRootForInvalidXml)
{
    XmlElement* root = nullptr;
    EXPECT_EQ(ER_EOF, XmlElement::GetRoot("InvalidXml", &root));
}

TEST(XmlElement, ShouldPassGetRootForValidInput)
{
    XmlElement* root = nullptr;
    EXPECT_EQ(ER_OK, XmlElement::GetRoot(VALID_ROOT_XML, &root));
    delete root;
}

TEST_F(XmlElementTest, ShouldReturnSameXmlAsGenerate)
{
    EXPECT_EQ(parent->Generate(), parent->ToString());
}

TEST_F(XmlElementTest, ShouldAddChild)
{
    parent->AddChild(child);

    EXPECT_EQ(1U, parent->GetChildren().size());
    EXPECT_EQ(child->GetName(), parent->GetChildren()[0]->GetName());
}


TEST(XmlElement, SetName)
{
    XmlElement root;
    EXPECT_EQ(String::Empty, root.GetName().c_str());
    root.SetName("root");
    EXPECT_STREQ("root", root.GetName().c_str());
}

TEST(XmlElement, GetName)
{
    XmlElement root = XmlElement("root");
    XmlElement* foo = new XmlElement("foo", &root, true);

    EXPECT_STREQ("root", root.GetName().c_str());
    EXPECT_STREQ("foo", foo->GetName().c_str());
}

TEST(XmlElement, AddAttribute)
{
    XmlElement root = XmlElement("root");

    root.AddAttribute("first", "Hello");
    root.AddAttribute("second", "World");

    EXPECT_STREQ("Hello", root.GetAttribute("first").c_str());
    EXPECT_STREQ("World", root.GetAttribute("second").c_str());
}

TEST(XmlElement, GetParent)
{
    XmlElement root = XmlElement("root");
    XmlElement* foo = new XmlElement("foo", &root, true);
    XmlElement* first = new XmlElement("value", foo, true);
    XmlElement* second = new XmlElement("value", foo, true);

    EXPECT_TRUE(NULL == root.GetParent());
    EXPECT_STREQ("root", foo->GetParent()->GetName().c_str());
    EXPECT_STREQ("foo", first->GetParent()->GetName().c_str());
    EXPECT_STREQ("foo", second->GetParent()->GetName().c_str());
    EXPECT_STREQ("root", first->GetParent()->GetParent()->GetName().c_str());
    EXPECT_STREQ("root", second->GetParent()->GetParent()->GetName().c_str());
}

/*
 * This test makes sure that when the parent node is passed into the XmlElement
 * constructor the child node is properly added to the parents list of children.
 */
TEST(XmlElement, constructor_add_child_to_parent_node)
{
    XmlElement root = XmlElement("root");
    XmlElement foo = XmlElement("foo", &root);
    XmlElement first = XmlElement("value", &foo);
    XmlElement second = XmlElement("value", &foo);

    const XmlElement* node_ptr = root.GetChild("foo");
    ASSERT_TRUE(node_ptr != NULL);
    EXPECT_STREQ("foo", node_ptr->GetName().c_str());
    node_ptr = foo.GetChild("value");
    ASSERT_TRUE(node_ptr != NULL);
    EXPECT_STREQ("value", node_ptr->GetName().c_str());
}

TEST(XmlElement, CreateChild_GetChild)
{
    XmlElement root = XmlElement("root");
    XmlElement* foo = root.CreateChild("foo");
    foo->CreateChild("value");
    foo->CreateChild("value");

    EXPECT_STREQ("foo", root.GetChild("foo")->GetName().c_str());
    EXPECT_STREQ("value", foo->GetChild("value")->GetName().c_str());

    EXPECT_TRUE(NULL == root.GetChild("bar"));
}

TEST(XmlElement, GetChildren_of_root_node)
{
    XmlElement root = XmlElement("root");
    XmlElement* foo = root.CreateChild("foo");
    foo->CreateChild("value");
    foo->CreateChild("value");

    const std::vector<XmlElement*>& children = root.GetChildren();

    EXPECT_EQ(1U, children.size());
    EXPECT_STREQ("foo", children[0]->GetName().c_str());

    const std::vector<XmlElement*>& children2 = foo->GetChildren();

    EXPECT_EQ(2U, children2.size());
    EXPECT_STREQ("value", children2[0]->GetName().c_str());
    EXPECT_STREQ("value", children2[1]->GetName().c_str());
}

TEST(XmlElement, GetChildren_by_name)
{
    XmlElement root = XmlElement("root");
    XmlElement* foo = root.CreateChild("foo");
    foo->CreateChild("value");
    foo->CreateChild("value");

    std::vector<const XmlElement*> children = root.GetChildren("foo");

    EXPECT_EQ(1U, children.size());
    EXPECT_STREQ("foo", children[0]->GetName().c_str());

    std::vector<const XmlElement*> children2 = foo->GetChildren("value");

    EXPECT_EQ(2U, children2.size());
    EXPECT_STREQ("value", children2[0]->GetName().c_str());
    EXPECT_STREQ("value", children2[1]->GetName().c_str());

}


TEST(XmlElement, Parse_double_quote)
{
    String xml = "<config>\
                      <foo>\
                          <value first=\"hello\"/>\
                          <value second=\"world\"/>\
                      </foo>\
                  </config>";
    StringSource source(xml);
    XmlParseContext pc(source);
    EXPECT_EQ(ER_OK, XmlElement::Parse(pc));
    const XmlElement* root = pc.GetRoot();

    ASSERT_TRUE(root != NULL);
    EXPECT_STREQ("config", root->GetName().c_str());
    EXPECT_STREQ("foo", root->GetChild("foo")->GetName().c_str());
    EXPECT_STREQ("value", root->GetChild("foo")->GetChildren()[0]->GetName().c_str());
    EXPECT_STREQ("value", root->GetChild("foo")->GetChildren()[1]->GetName().c_str());

    EXPECT_STREQ("hello", root->GetChild("foo")->GetChildren()[0]->GetAttribute("first").c_str());
    EXPECT_STREQ("world", root->GetChild("foo")->GetChildren()[1]->GetAttribute("second").c_str());

}

TEST(XmlElement, Parse_single_quote)
{
    String xml = "<config>\
                      <foo>\
                          <value first='hello'/>\
                          <value second='world'/>\
                      </foo>\
                  </config>";
    StringSource source(xml);
    XmlParseContext pc(source);
    EXPECT_EQ(ER_OK, XmlElement::Parse(pc));
    const XmlElement* root = pc.GetRoot();

    ASSERT_TRUE(root != NULL);
    EXPECT_STREQ("config", root->GetName().c_str());

    EXPECT_STREQ("foo", root->GetChild("foo")->GetName().c_str());
    EXPECT_STREQ("value", root->GetChild("foo")->GetChildren()[0]->GetName().c_str());
    EXPECT_STREQ("value", root->GetChild("foo")->GetChildren()[1]->GetName().c_str());

    EXPECT_STREQ("hello", root->GetChild("foo")->GetChildren()[0]->GetAttribute("first").c_str());
    EXPECT_STREQ("world", root->GetChild("foo")->GetChildren()[1]->GetAttribute("second").c_str());
}

TEST(XmlElement, Parse_mixed_quote)
{
    String xml = "<config>\
                      <foo>\
                          <value first='<bar value=\"hello\"/>'/>\
                          <value second=\"<bar value='world'/>\"/>\
                      </foo>\
                  </config>";
    StringSource source(xml);
    XmlParseContext pc(source);
    EXPECT_EQ(ER_OK, XmlElement::Parse(pc));
    const XmlElement* root = pc.GetRoot();

    ASSERT_TRUE(root != NULL);
    EXPECT_STREQ("config", root->GetName().c_str());

    EXPECT_STREQ("foo", root->GetChild("foo")->GetName().c_str());
    EXPECT_STREQ("value", root->GetChild("foo")->GetChildren()[0]->GetName().c_str());
    EXPECT_STREQ("value", root->GetChild("foo")->GetChildren()[1]->GetName().c_str());

    EXPECT_STREQ("<bar value=\"hello\"/>", root->GetChild("foo")->GetChildren()[0]->GetAttribute("first").c_str());
    EXPECT_STREQ("<bar value='world'/>", root->GetChild("foo")->GetChildren()[1]->GetAttribute("second").c_str());
}

TEST(XmlElement, GetPath)
{
    String xml = "<config>\
                      <foo>\
                          <value first='hello'/>\
                          <value second='world'/>\
                      </foo>\
                  </config>";
    StringSource source(xml);
    XmlParseContext pc(source);
    EXPECT_EQ(ER_OK, XmlElement::Parse(pc));
    const XmlElement* root = pc.GetRoot();

    EXPECT_STREQ("hello", root->GetPath("foo/value")[0]->GetAttribute("first").c_str());
    EXPECT_STREQ("world", root->GetPath("foo/value")[1]->GetAttribute("second").c_str());

    EXPECT_STREQ("hello", root->GetPath("foo/value@first")[0]->GetAttribute("first").c_str());
    EXPECT_STREQ("world", root->GetPath("foo/value@second")[0]->GetAttribute("second").c_str());
}

TEST(XmlElement, ParseInvalidXml)
{
    /* Test that the parser fails gracefully on invalid input. */
    String xml = "</ ";
    StringSource source(xml);
    XmlParseContext pc(source);
    EXPECT_EQ(ER_OK, XmlElement::Parse(pc));
    /* Note: XmlElement::Parse should return ER_MALFORMED_XML, not ER_OK.  See ASACORE-2902. */
}

TEST(XmlElement, GetRootInvalidXml)
{
    /* Test that GetRoot fails gracefully on invalid input. */
    String xml = "</ ";
    XmlElement* root = NULL;
    EXPECT_EQ(ER_OK, XmlElement::GetRoot(xml.c_str(), &root));
    /* Note: GetRoot should return ER_MALFORMED_XML, not ER_OK.  See ASACORE-2902. */

    EXPECT_EQ(String::Empty, root->GetName().c_str());
    EXPECT_EQ((size_t)0, root->GetChildren().size());
}

TEST_F(XmlElementTest, ShouldPassWithValidComment)
{
    String xml = "<config>\
                      <foo>\
                          <value first='hello'/>\
                          <!-- foo></foo -->\
                          <value second='world'/>\
                      </foo>\
                  </config>";

    EXPECT_EQ(ER_OK, XmlElement::GetRoot(xml.c_str(), &root));
    /*
     * Test that parsing the comment ended with the "-->" tag
     * and we can access the attributes of the following tag.
     * See ASACORE-3177
     */
    EXPECT_STREQ("world", root->GetChild("foo")->GetChildren()[1]->GetAttribute("second").c_str());

    delete root;

    xml = "<!-- Example: <config></config>. See docs  -->\
           <root/>";
    EXPECT_EQ(ER_OK, XmlElement::GetRoot(xml.c_str(), &root));

    delete root;

    xml = "<!-- TODO: Review -->\
           <root/>";
    EXPECT_EQ(ER_OK, XmlElement::GetRoot(xml.c_str(), &root));
}

TEST_F(XmlElementTest, ShouldFailWithInvalidComment)
{
    String xml = "<config>\
                      <foo>\
                          <value first='hello'/>\
                          <!-- \
                          <value second='world'/>\
                      </foo>\
                  </config>";
    EXPECT_EQ(ER_EOF, XmlElement::GetRoot(xml.c_str(), &root));
}

TEST_F(XmlElementTest, ShouldPassWithTextDecleration)
{
    String xml = "<?xml version='1.0'?> \
                  <config>\
                      <foo>\
                          <value first='hello'/>\
                          <value second='world'/>\
                      </foo>\
                  </config>";
    EXPECT_EQ(ER_OK, XmlElement::GetRoot(xml.c_str(), &root));
}

TEST_F(XmlElementTest, ShouldPassWithDoctype)
{
    String xml = "<!DOCTYPE busconfig PUBLIC '-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN' \
                   'http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd'> \
                  <config>\
                      <foo>\
                          <value first='hello'/>\
                          <value second='world'/>\
                      </foo>\
                  </config>";
    EXPECT_EQ(ER_OK, XmlElement::GetRoot(xml.c_str(), &root));
}

TEST_F(XmlElementTest, ShouldFailWithInvalidTextDecleration)
{

    /* This is not a valid text declereation. This should fail gracefully */
    String xml = "<?>";
    EXPECT_EQ(ER_XML_MALFORMED, XmlElement::GetRoot(xml.c_str(), &root));
}

TEST_F(XmlElementTest, ShouldFailWithInvalidDefinitionTag)
{
    /* This is not a valid doctype or comment. This should fail gracefully. */
    String xml = "<!>";
    EXPECT_EQ(ER_XML_MALFORMED, XmlElement::GetRoot(xml.c_str(), &root));
}
