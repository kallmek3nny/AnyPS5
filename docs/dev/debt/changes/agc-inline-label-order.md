# libSceAgcDriver

## add Functional

- [Inline label-store ordering](../../../core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp) (libSceAgcDriver) - the cost of a transfer/host dependency between separately recorded inline updates is unmeasured. Host polling without a Vulkan fence or event wait retains the existing driver-behavior assumption for the batch's earlier writes; PS5 label-ordering behavior has not been measured.
