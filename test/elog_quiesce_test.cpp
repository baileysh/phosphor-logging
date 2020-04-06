#include "config.h"

#include "elog_entry.hpp"
#include "log_manager.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace phosphor
{
namespace logging
{
namespace test
{

class TestQuiesceOnError : public testing::Test
{
  public:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus::bus mockedBus = sdbusplus::get_mocked_new(&sdbusMock);
    phosphor::logging::internal::Manager manager;

    TestQuiesceOnError() : manager(mockedBus, OBJ_INTERNAL)
    {
    }
};

// Test that false is returned when no callout is present in the log
TEST_F(TestQuiesceOnError, testNoCallout)
{
    uint32_t id = 99;
    uint64_t timestamp{100};
    std::string message{"test error"};
    std::string fwLevel{"level42"};
    std::vector<std::string> testData{"no", "callout"};
    phosphor::logging::AssociationList associations{};

    Entry elog{mockedBus,
               std::string(OBJ_ENTRY) + '/' + std::to_string(id),
               id,
               timestamp,
               Entry::Level::Informational,
               std::move(message),
               std::move(testData),
               std::move(associations),
               fwLevel,
               manager};

    EXPECT_EQ(manager.isCalloutPresent(elog), false);
}

// Test that trues is returned when a callout is present in the log
TEST_F(TestQuiesceOnError, testCallout)
{
    uint32_t id = 99;
    uint64_t timestamp{100};
    std::string message{"test error"};
    std::string fwLevel{"level42"};
    std::vector<std::string> testData{
        "CALLOUT_INVENTORY_PATH=/xyz/openbmc_project/inventory/system/chassis/"
        "motherboard/powersupply0/"};
    phosphor::logging::AssociationList associations{};

    Entry elog{mockedBus,
               std::string(OBJ_ENTRY) + '/' + std::to_string(id),
               id,
               timestamp,
               Entry::Level::Informational,
               std::move(message),
               std::move(testData),
               std::move(associations),
               fwLevel,
               manager};

    EXPECT_EQ(manager.isCalloutPresent(elog), true);
}

// Test that no blocking errors are created when no callout
TEST_F(TestQuiesceOnError, testNoBlockingErrorsCreated)
{
    uint32_t id = 99;
    uint64_t timestamp{100};
    std::string message{"test error"};
    std::string fwLevel{"level42"};
    std::vector<std::string> testData{"no", "callout"};
    phosphor::logging::AssociationList associations{};

    Entry elog{mockedBus,
               std::string(OBJ_ENTRY) + '/' + std::to_string(id),
               id,
               timestamp,
               Entry::Level::Informational,
               std::move(message),
               std::move(testData),
               std::move(associations),
               fwLevel,
               manager};

    manager.checkQuiesceOnError(elog);
    EXPECT_EQ(manager.getBlockingErrSize(), 0);
}

// Test that a blocking error is created on entry with callout
TEST_F(TestQuiesceOnError, testBlockingErrorsCreated)
{
    uint32_t id = 100;
    uint64_t timestamp{100};
    std::string message{"test error"};
    std::string fwLevel{"level42"};
    std::vector<std::string> testData{
        "CALLOUT_INVENTORY_PATH=/xyz/openbmc_project/inventory/system/chassis/"
        "motherboard/powersupply0/"};
    phosphor::logging::AssociationList associations{};

    // Ensure D-Bus object created for this blocking error
    // First allow any number of sd_bus_emit_object_added calls
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(testing::_, testing::_))
        .Times(testing::AnyNumber());
    // Second verify the new block100 object is created once
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_object_added(
                    testing::_, testing::HasSubstr(
                                    "/xyz/openbmc_project/logging/block100")))
        .Times(1);

    Entry elog{mockedBus,
               std::string(OBJ_ENTRY) + '/' + std::to_string(id),
               id,
               timestamp,
               Entry::Level::Informational,
               std::move(message),
               std::move(testData),
               std::move(associations),
               fwLevel,
               manager};

    manager.checkQuiesceOnError(elog);
    // Created error with callout so expect a blocking error now
    EXPECT_EQ(manager.getBlockingErrSize(), 1);
}

} // namespace test
} // namespace logging
} // namespace phosphor