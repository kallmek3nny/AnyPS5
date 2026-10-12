from pathlib import Path


def replace(path: str, old: str, new: str, expected: int = 1) -> None:
    file = Path(path)
    text = file.read_text()
    count = text.count(old)
    if count != expected:
        raise SystemExit(f"{path}: expected {expected} matches, found {count}")
    file.write_text(text.replace(old, new))


replace(
    "core/relinker/domain/include/domain/Types.hpp",
    "    std::int64_t Addend;\n};\n",
    "    std::int64_t Addend;\n    bool Weak = false;\n};\n",
)

replace(
    "core/relinker/relinker/src/pipeline/RelinkerPipeline.cpp",
    "            if (symOff < dynSymTabOffset || symOff > raw.size() || raw.size() - symOff < 4)\n",
    "            if (symOff < dynSymTabOffset || symOff > raw.size() || raw.size() - symOff < 5)\n",
)
replace(
    "core/relinker/relinker/src/pipeline/RelinkerPipeline.cpp",
    "            std::uint32_t nameOff = 0;\n            std::memcpy(&nameOff, raw.data() + symOff, 4);\n\n            const auto name = readCStr(nameOff);\n            nidRefs.push_back({name, Domain::ImportModule(name, importModules, neededLibraries), relType, pos, rOffset, rAddend});\n",
    "            std::uint32_t nameOff = 0;\n            std::memcpy(&nameOff, raw.data() + symOff, 4);\n            const bool weak = (raw[symOff + 4] >> 4) == STB_WEAK;\n\n            const auto name = readCStr(nameOff);\n            nidRefs.push_back({name, Domain::ImportModule(name, importModules, neededLibraries), relType, pos, rOffset, rAddend, weak});\n",
)

replace(
    "core/relinker/relinker/src/output/SysVDynamicSectionBuilder.cpp",
    "        const auto info = static_cast<std::uint8_t>((STB_GLOBAL << 4) | STT_FUNC);\n",
    "        const auto info = static_cast<std::uint8_t>(((ref.Weak ? STB_WEAK : STB_GLOBAL) << 4) | STT_FUNC);\n",
    expected=2,
)

replace(
    "core/relinker/elfpatcher/include/elfpatcher/windows/WindowsPeFormat.hpp",
    "    std::uint32_t RelocationType = 1;\n    std::string Library;\n};\n",
    "    std::uint32_t RelocationType = 1;\n    std::string Library;\n    bool Weak = false;\n};\n",
)

replace(
    "core/relinker/elfpatcher/src/windows/WindowsRelocationBuilder.cpp",
    "#include <elfpatcher/windows/WindowsRelocationBuilder.hpp>\n#include <io/BufferUtils.hpp>\n",
    "#include <elfpatcher/windows/WindowsRelocationBuilder.hpp>\n#include <elfpatcher/general/ElfConstants.hpp>\n#include <io/BufferUtils.hpp>\n",
)
replace(
    "core/relinker/elfpatcher/src/windows/WindowsRelocationBuilder.cpp",
    "            const auto name = ReadString(dynamicSection.DynStrData, Io::ReadU32(dynamicSection.DynSymData, symbolOffset));\n            if (name.empty())\n                throw Domain::RelinkerException(\"Empty import symbol name\", target);\n",
    "            const auto name = ReadString(dynamicSection.DynStrData, Io::ReadU32(dynamicSection.DynSymData, symbolOffset));\n            if (name.empty())\n                throw Domain::RelinkerException(\"Empty import symbol name\", target);\n            const bool weak = (dynamicSection.DynSymData[symbolOffset + 4] >> 4) == STB_WEAK;\n",
)
replace(
    "core/relinker/elfpatcher/src/windows/WindowsRelocationBuilder.cpp",
    "            result.Imports.push_back({name, rva, addend});\n            if (const auto module = dynamicSection.ImportModules.find(target); module != dynamicSection.ImportModules.end())\n",
    "            result.Imports.push_back({name, rva, addend});\n            result.Imports.back().Weak = weak;\n            if (const auto module = dynamicSection.ImportModules.find(target); module != dynamicSection.ImportModules.end())\n",
)

replace(
    "core/relinker/elfpatcher/src/windows/WindowsEntryStubBuilder.cpp",
    "        code.Emit({0x48, 0x83, 0xc7, 4, 0xff, 0xcd});\n        code.Rip({0x0f, 0x85}, search);\n        if (lazyBinding) {\n",
    "        code.Emit({0x48, 0x83, 0xc7, 4, 0xff, 0xcd});\n        code.Rip({0x0f, 0x85}, search);\n        if (imports[index].Weak) {\n            code.Emit({0x31, 0xc0});\n            if (imports[index].Addend != 0) {\n                code.Emit({0x48, 0xb8});\n                code.U64(imports[index].Addend);\n            }\n            guestStartup.WriteImport(code, imports[index], handles);\n            const auto done = code.Branch({0xe9});\n            code.PatchBranch(resolved, code.GetRva());\n            if (imports[index].Addend != 0) {\n                code.Emit({0x48, 0xba});\n                code.U64(imports[index].Addend);\n                code.Emit({0x48, 0x01, 0xd0});\n            }\n            guestStartup.WriteImport(code, imports[index], handles);\n            code.PatchBranch(done, code.GetRva());\n            continue;\n        }\n        if (lazyBinding) {\n",
)

replace(
    "core/relinker/relinker/tests/StrictNidFilterTests.cpp",
    "}\n\n}\n\nint main() {\n",
    "}\n\nvoid weakImportBinding() {\n    Relinker::SysVDynamicSectionBuilder builder;\n    const std::vector<Relinker::NidReference> references = {\n        {\"weak\", {}, 6, 0x100, 0x2000, 0, true},\n        {\"strong\", {}, 6, 0x118, 0x2008, 0}\n    };\n    const auto section = builder.BuildDynamicSection(references, {}, 0, 0);\n    require(section.DynSymData.size() == 72, \"Weak import test built the wrong dynamic symbol count\");\n    require((section.DynSymData[28] >> 4) == 2, \"Weak import binding was not preserved\");\n    require((section.DynSymData[52] >> 4) == 1, \"Strong import binding changed\");\n}\n\n}\n\nint main() {\n",
)
replace(
    "core/relinker/relinker/tests/StrictNidFilterTests.cpp",
    "        sceDynlibDataOffsets();\n        std::cout << \"Strict NID filter tests passed\\n\";\n",
    "        sceDynlibDataOffsets();\n        weakImportBinding();\n        std::cout << \"Strict NID filter tests passed\\n\";\n",
)

replace(
    "core/relinker/relinker/tests/test_windows_import_modules.py",
    "def executable(owner, symbol='shared#A#B', module_name=None, extra_dependencies=()):\n",
    "def executable(owner, symbol='shared#A#B', module_name=None, extra_dependencies=(), weak=False):\n",
)
replace(
    "core/relinker/relinker/tests/test_windows_import_modules.py",
    "    struct.pack_into('<IBBHQQ', image, 0x4898, 1, 0x12, 0, 0, 0, 0)\n",
    "    struct.pack_into('<IBBHQQ', image, 0x4898, 1, 0x22 if weak else 0x12, 0, 0, 0, 0)\n",
)
replace(
    "core/relinker/relinker/tests/test_windows_import_modules.py",
    "    image[0x4000:0x4007] = b'\\xff\\x15' + struct.pack('<i', 0x4a20 - 0x4006) + b'\\xc3'\n",
    "    if weak:\n        image[0x4000:0x4012] = b'\\x48\\x8b\\x05' + struct.pack('<i', 0x4a20 - 0x4007) + b'\\x48\\x85\\xc0\\x74\\x03\\xff\\xd0\\xc3\\x31\\xc0\\xc3'\n    else:\n        image[0x4000:0x4007] = b'\\xff\\x15' + struct.pack('<i', 0x4a20 - 0x4006) + b'\\xc3'\n",
)
replace(
    "core/relinker/relinker/tests/test_windows_import_modules.py",
    "        def convert(name, owner, symbol='shared#A#B', guest_owner=None, module_name=None, provider_name='b.prx', extra_dependencies=(), windows_gui=False):\n",
    "        def convert(name, owner, symbol='shared#A#B', guest_owner=None, module_name=None, provider_name='b.prx', extra_dependencies=(), windows_gui=False, weak=False):\n",
)
replace(
    "core/relinker/relinker/tests/test_windows_import_modules.py",
    "            source.write_bytes(executable(owner, symbol, module_name, extra_dependencies))\n",
    "            source.write_bytes(executable(owner, symbol, module_name, extra_dependencies, weak))\n",
)
replace(
    "core/relinker/relinker/tests/test_windows_import_modules.py",
    "        result, output = convert('missing-symbol', 'a.prx', 'absent#A#B')\n        assert result.returncode == 0, result.stderr\n",
    "        result, output = convert('missing-weak-symbol', 'a.prx', 'absent#A#B', weak=True)\n        assert result.returncode == 0, result.stderr\n        if os.name == 'nt':\n            run = subprocess.run([str(output)], capture_output=True, text=True, timeout=30)\n            assert run.returncode == 0 and 'unresolved ELF import absent' not in run.stderr, (run.returncode, run.stdout, run.stderr)\n\n        result, output = convert('missing-symbol', 'a.prx', 'absent#A#B')\n        assert result.returncode == 0, result.stderr\n",
)
