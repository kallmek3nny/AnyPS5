#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <map>
#include <mutex>
#include <span>
#include <string_view>
#include <vector>

using namespace AgcDriver::Graphics;

namespace {

struct Command {
    bool update = false;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    std::uint32_t value = 0;
    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;
    VkAccessFlags sourceAccess = 0;
    VkAccessFlags destinationAccess = 0;
};

struct MockDevice {
    std::uintptr_t next = 1;
    std::map<VkFence, bool> fences;
    std::vector<Command> commands;
};

MockDevice mock;
int failures = 0;

void Expect(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "inline store ordering: %s\n", message);
    ++failures;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateCommands(VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer* commands) {
    *commands = reinterpret_cast<VkCommandBuffer>(mock.next++);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL FreeCommands(VkDevice, VkCommandPool, std::uint32_t, const VkCommandBuffer*) {}

VKAPI_ATTR VkResult VKAPI_CALL CreateFence(VkDevice, const VkFenceCreateInfo*, const VkAllocationCallbacks*, VkFence* fence) {
    *fence = reinterpret_cast<VkFence>(mock.next++);
    mock.fences[*fence] = false;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL DestroyFence(VkDevice, VkFence fence, const VkAllocationCallbacks*) { mock.fences.erase(fence); }
VKAPI_ATTR VkResult VKAPI_CALL FenceStatus(VkDevice, VkFence fence) { return mock.fences.at(fence) ? VK_SUCCESS : VK_NOT_READY; }

VKAPI_ATTR VkResult VKAPI_CALL WaitFences(VkDevice, std::uint32_t count, const VkFence* fences, VkBool32, std::uint64_t) {
    for (std::uint32_t i = 0; i < count; ++i) mock.fences.at(fences[i]) = true;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetFences(VkDevice, std::uint32_t count, const VkFence* fences) {
    for (std::uint32_t i = 0; i < count; ++i) mock.fences.at(fences[i]) = false;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL BeginCommands(VkCommandBuffer, const VkCommandBufferBeginInfo*) { return VK_SUCCESS; }
VKAPI_ATTR VkResult VKAPI_CALL EndCommands(VkCommandBuffer) { return VK_SUCCESS; }
VKAPI_ATTR VkResult VKAPI_CALL Submit(VkQueue, std::uint32_t, const VkSubmitInfo*, VkFence) { return VK_SUCCESS; }
VKAPI_ATTR VkResult VKAPI_CALL WaitIdle(VkDevice) { return VK_SUCCESS; }

VKAPI_ATTR void VKAPI_CALL Update(VkCommandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* data) {
    Command command;
    command.update = true;
    command.buffer = buffer;
    command.offset = offset;
    command.size = size;
    if (size == sizeof(command.value)) std::memcpy(&command.value, data, sizeof(command.value));
    mock.commands.push_back(command);
}

VKAPI_ATTR void VKAPI_CALL Barrier(VkCommandBuffer, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage,
    VkDependencyFlags, std::uint32_t count, const VkMemoryBarrier* barriers, std::uint32_t,
    const VkBufferMemoryBarrier*, std::uint32_t, const VkImageMemoryBarrier*) {
    Command command;
    command.sourceStage = sourceStage;
    command.destinationStage = destinationStage;
    for (std::uint32_t i = 0; i < count; ++i) {
        command.sourceAccess |= barriers[i].srcAccessMask;
        command.destinationAccess |= barriers[i].dstAccessMask;
    }
    mock.commands.push_back(command);
}

VKAPI_ATTR void VKAPI_CALL BeginQuery(VkCommandBuffer, VkQueryPool, std::uint32_t, VkQueryControlFlags) {}
VKAPI_ATTR void VKAPI_CALL EndQuery(VkCommandBuffer, VkQueryPool, std::uint32_t) {}
VKAPI_ATTR void VKAPI_CALL ResetQuery(VkCommandBuffer, VkQueryPool, std::uint32_t, std::uint32_t) {}
VKAPI_ATTR void VKAPI_CALL CopyQuery(VkCommandBuffer, VkQueryPool, std::uint32_t, std::uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags) {}
VKAPI_ATTR void VKAPI_CALL PushConstants(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, std::uint32_t, std::uint32_t, const void*) {}
VKAPI_ATTR void VKAPI_CALL Dispatch(VkCommandBuffer, std::uint32_t, std::uint32_t, std::uint32_t) {}

PFN_vkVoidFunction VKAPI_CALL Procedure(VkDevice, const char* name) {
    static const std::map<std::string_view, PFN_vkVoidFunction> functions{
        {"vkAllocateCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(AllocateCommands)},
        {"vkFreeCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(FreeCommands)},
        {"vkCreateFence", reinterpret_cast<PFN_vkVoidFunction>(CreateFence)},
        {"vkDestroyFence", reinterpret_cast<PFN_vkVoidFunction>(DestroyFence)},
        {"vkGetFenceStatus", reinterpret_cast<PFN_vkVoidFunction>(FenceStatus)},
        {"vkWaitForFences", reinterpret_cast<PFN_vkVoidFunction>(WaitFences)},
        {"vkResetFences", reinterpret_cast<PFN_vkVoidFunction>(ResetFences)},
        {"vkBeginCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(BeginCommands)},
        {"vkEndCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(EndCommands)},
        {"vkQueueSubmit", reinterpret_cast<PFN_vkVoidFunction>(Submit)},
        {"vkDeviceWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(WaitIdle)},
        {"vkCmdUpdateBuffer", reinterpret_cast<PFN_vkVoidFunction>(Update)},
        {"vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(Barrier)},
        {"vkCmdBeginQuery", reinterpret_cast<PFN_vkVoidFunction>(BeginQuery)},
        {"vkCmdEndQuery", reinterpret_cast<PFN_vkVoidFunction>(EndQuery)},
        {"vkCmdResetQueryPool", reinterpret_cast<PFN_vkVoidFunction>(ResetQuery)},
        {"vkCmdCopyQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(CopyQuery)},
        {"vkCmdPushConstants", reinterpret_cast<PFN_vkVoidFunction>(PushConstants)},
        {"vkCmdDispatch", reinterpret_cast<PFN_vkVoidFunction>(Dispatch)},
    };
    const auto found = functions.find(name);
    return found == functions.end() ? nullptr : found->second;
}

struct Store {
    VkBuffer buffer;
    VkDeviceSize offset;
    std::uint32_t value;
};

void Check(const char* name, std::span<const Store> stores) {
    mock = {};
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    const auto before = Recorder::StoreCounts();
    Context context{};
    context.device = reinterpret_cast<VkDevice>(mock.next++);
    context.queue = reinterpret_cast<VkQueue>(mock.next++);
    context.deviceProc = Procedure;
    {
        Recorder recorder(context);
        for (const auto& store : stores)
            recorder.RecordStore(store.buffer, store.offset, std::as_bytes(std::span(&store.value, 1)), 0);
        recorder.Sync();
    }
    std::vector<std::size_t> updates;
    std::size_t barriers = 0;
    for (std::size_t index = 0; index < mock.commands.size(); ++index)
        if (mock.commands[index].update) updates.push_back(index);
        else ++barriers;
    const auto after = Recorder::StoreCounts();
    std::printf("%s: stores=%zu updates=%zu LabelRun_barriers=%zu waw_barriers=%llu\n", name, stores.size(),
        updates.size(), barriers, static_cast<unsigned long long>(after.wawBarriers - before.wawBarriers));
    Expect(barriers == (stores.empty() ? 0 : stores.size() + 1), "the run emitted an unexpected number of barriers");
    Expect(updates.size() == stores.size(), "separate stores did not produce the expected updates");
    if (updates.size() != stores.size()) return;
    for (std::size_t index = 0; index < stores.size(); ++index) {
        const auto& update = mock.commands[updates[index]];
        Expect(update.buffer == stores[index].buffer && update.offset == stores[index].offset &&
            update.size == sizeof(std::uint32_t) && update.value == stores[index].value,
            "a store changed its destination, order or payload");
        if (index == 0) continue;
        bool ordered = false;
        for (auto position = updates[index - 1] + 1; position < updates[index]; ++position) {
            const auto& barrier = mock.commands[position];
            constexpr auto stages = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT;
            constexpr auto access = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT;
            ordered |= !barrier.update && (barrier.sourceStage & VK_PIPELINE_STAGE_TRANSFER_BIT) != 0 &&
                (barrier.sourceAccess & VK_ACCESS_TRANSFER_WRITE_BIT) != 0 &&
                (barrier.destinationStage & stages) == stages && (barrier.destinationAccess & access) == access;
        }
        Expect(ordered, "a later update lacks a transfer/host dependency on the earlier store");
    }
    Expect(mock.fences.empty(), "the recorder retained a fence after synchronization");
}

}

int main() {
    try {
        const auto first = reinterpret_cast<VkBuffer>(std::uintptr_t{0x1000});
        const auto second = reinterpret_cast<VkBuffer>(std::uintptr_t{0x2000});
        const std::array single{Store{first, 0, 0x12345678}};
        const std::array separate{Store{first, 0, 0x11111111}, Store{second, 0, 0x22222222}, Store{first, 0, 0x33333333}};
        const std::array disjoint{Store{first, 0, 0x44444444}, Store{first, 8, 0x55555555}};
        Check("empty", {});
        Check("single", single);
        Check("separate buffers with overlap", separate);
        Check("disjoint ranges", disjoint);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "inline store ordering: %s\n", error.what());
        return 1;
    }
    if (failures != 0) return 1;
    std::puts("inline store ordering checks passed");
    return 0;
}
