/* Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <gtest/gtest.h>

extern "C" {
#include <supla_esp_cfg.h>

unsigned int ICACHE_FLASH_ATTR cfg_str2centInt(const char *str,
                                               unsigned short decLimit);
unsigned int ICACHE_FLASH_ATTR cfg_str2int(const char *str);
}

class ConfigurationTestsF : public ::testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST(ConfigurationTests, CheckCfgStructureSize) {
  EXPECT_LE(sizeof(SuplaEspCfg), 1024);
}

TEST(ConfigurationTests, Str2IntConversion) {
  ASSERT_EQ(0, cfg_str2int("abcd"));
  ASSERT_EQ(12, cfg_str2int("1a2"));
  ASSERT_EQ(0, cfg_str2int("0"));
  ASSERT_EQ(123, cfg_str2int("123"));
  ASSERT_EQ(-123, cfg_str2int("-123"));
  ASSERT_EQ(2147483647, cfg_str2int("2147483647"));
  ASSERT_EQ((unsigned int)4294967295, cfg_str2int("4294967295"));
}

TEST(ConfigurationTests, Str2CentIntConversion) {
  ASSERT_EQ(0, cfg_str2centInt("abcd", 2));
  ASSERT_EQ(1200, cfg_str2centInt("1a2", 2));
  ASSERT_EQ(0, cfg_str2centInt("0", 2));
  ASSERT_EQ(12300, cfg_str2centInt("123", 2));
  ASSERT_EQ(12300, cfg_str2centInt("-123", 2));
  ASSERT_EQ(2147483647, cfg_str2centInt("21474836.47", 2));
  ASSERT_EQ((unsigned int)4294967295, cfg_str2centInt("42949672.95", 2));
  ASSERT_EQ(12, cfg_str2centInt("0.125", 2));
}

TEST(ConfigurationTests, Str2CentIntConversionWithoutDecimals) {
  ASSERT_EQ(0, cfg_str2centInt("abcd", 0));
  ASSERT_EQ(12, cfg_str2centInt("1a2", 0));
  ASSERT_EQ(0, cfg_str2centInt("0", 0));
  ASSERT_EQ(123, cfg_str2centInt("123", 0));
  ASSERT_EQ(123, cfg_str2centInt("-123", 0));
  ASSERT_EQ(21474836, cfg_str2centInt("21474836.47", 0));
  ASSERT_EQ((unsigned int)42949672, cfg_str2centInt("42949672.95", 0));
  ASSERT_EQ(2147483647, cfg_str2centInt("2147483647", 0));
  ASSERT_EQ((unsigned int)4294967295, cfg_str2centInt("4294967295", 0));
  ASSERT_EQ(0, cfg_str2centInt("0.125", 0));
}

TEST(ConfigurationTests, Str2CentIntConversionWithThreeDecimalPlaces) {
  ASSERT_EQ(0, cfg_str2centInt("abcd", 3));
  ASSERT_EQ(12000, cfg_str2centInt("1a2", 3));
  ASSERT_EQ(0, cfg_str2centInt("0", 3));
  ASSERT_EQ(123000, cfg_str2centInt("123", 3));
  ASSERT_EQ(123000, cfg_str2centInt("-123", 3));
  ASSERT_EQ(2147483647, cfg_str2centInt("2147483.647", 3));
  ASSERT_EQ((unsigned int)4294967295, cfg_str2centInt("4294967.295", 3));
  ASSERT_EQ(125, cfg_str2centInt("0.125", 3));
}
