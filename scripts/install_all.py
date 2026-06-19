#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

ONNX_VERSION = "1.24.1"
DEFAULT_WINDOWS_TOOLSET = "v145"

ASR_MODELS = [
    {
        "label": "ASR model (English/Chinese)",
        "lang": "en",
        "url": "https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-streaming-zipformer-bilingual-zh-en-2023-02-20.tar.bz2",
        "dir": "online-zipformer-bilingual-zh-en-2023-02-20",
    },
    {
        "label": "ASR model (German)",
        "lang": "de",
        "url": "https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-streaming-zipformer-de-kroko-2025-08-06.tar.bz2",
        "dir": "sherpa-onnx-streaming-zipformer-de-kroko-2025-08-06",
    },
]

ASR_EXAMPLES = [
    "example_asr",
    "example_asr_buffer",
]

TTS_MODELS = [
    {
        "label": "TTS model (English)",
        "lang": "en",
        "base_url": "https://huggingface.co/csukuangfj/vits-piper-en_US-amy-low/resolve/main",
        "target": "example_tts/bin/data/models/vits-piper-en_US-amy-low",
        "files": {
            "model.onnx": "en_US-amy-low.onnx",
            "lexicon.txt": "lexicon.txt",
            "tokens.txt": "tokens.txt",
            "en_US-amy-low.onnx.json": "en_US-amy-low.onnx.json",
        },
        "optional": {"lexicon.txt"},
        "clone_for_espeak": "https://huggingface.co/csukuangfj/vits-piper-en_US-amy-low",
    },
    {
        "label": "TTS model (German)",
        "lang": "de",
        "base_url": "https://huggingface.co/csukuangfj/vits-piper-de_DE-thorsten-low/resolve/main",
        "target": "example_tts/bin/data/models/vits-piper-de_DE-thorsten-low",
        "files": {
            "model.onnx": "de_DE-thorsten-low.onnx",
            "tokens.txt": "tokens.txt",
            "de_DE-thorsten-low.onnx.json": "de_DE-thorsten-low.onnx.json",
        },
        "optional": set(),
        "clone_for_espeak": "https://huggingface.co/csukuangfj/vits-piper-de_DE-thorsten-low",
    },
]


def log(message: str = "") -> None:
    print(message, flush=True)


def run(cmd: list[str], cwd: Path | None = None) -> None:
    log("$ " + " ".join(cmd))
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def require_tool(name: str) -> None:
    if shutil.which(name) is None:
        raise SystemExit(f"Missing required tool: {name}. Please install it and run this again.")


def host_os_arch() -> tuple[str, str, str, str]:
    system = platform.system()
    machine = platform.machine().lower()

    if machine in {"x86_64", "amd64"}:
        arch = "x86_64"
        win_arch = "x64"
    elif machine in {"arm64", "aarch64"}:
        arch = "arm64"
        win_arch = "ARM64"
    else:
        raise SystemExit(f"Unsupported architecture: {platform.machine()}")

    if system == "Linux":
        uname_os = "Linux"
    elif system == "Darwin":
        uname_os = "Darwin"
    elif system == "Windows":
        uname_os = "Windows"
    else:
        raise SystemExit(f"Unsupported OS: {system}")

    return system, uname_os, arch, win_arch


def download(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    log(f"Downloading {url}")
    with urllib.request.urlopen(url) as response, dest.open("wb") as f:
        shutil.copyfileobj(response, f)


def extract_archive(archive: Path, dest: Path, strip_components: int = 0) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    name = archive.name.lower()

    if name.endswith(".zip"):
        with zipfile.ZipFile(archive) as zf:
            for member in zf.namelist():
                parts = Path(member).parts[strip_components:]
                if not parts:
                    continue
                target = dest.joinpath(*parts)
                if member.endswith("/"):
                    target.mkdir(parents=True, exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with zf.open(member) as src, target.open("wb") as out:
                        shutil.copyfileobj(src, out)
    else:
        with tarfile.open(archive, "r:*") as tf:
            for member in tf.getmembers():
                parts = Path(member.name).parts[strip_components:]
                if not parts:
                    continue
                member.name = str(Path(*parts))
                tf.extract(member, dest)


def copy_matching(src_dir: Path, dst_dir: Path, suffixes: tuple[str, ...]) -> None:
    if not src_dir.exists():
        return
    dst_dir.mkdir(parents=True, exist_ok=True)
    for item in src_dir.iterdir():
        name = item.name
        if item.is_file() and (name.endswith(suffixes) or ".so." in name or ".dylib." in name):
            shutil.copy2(item, dst_dir / name)


def patch_vcxproj_toolset(vcxproj_path: Path, toolset: str) -> None:
    if not vcxproj_path.exists():
        return

    text = vcxproj_path.read_text(encoding="utf-8", errors="ignore")

    if "<PlatformToolset>" in text:
        text = re.sub(
            r"<PlatformToolset>.*?</PlatformToolset>",
            f"<PlatformToolset>{toolset}</PlatformToolset>",
            text,
            flags=re.DOTALL,
        )
    else:
        text = re.sub(
            r"(<CharacterSet>Unicode</CharacterSet>)",
            rf"\1\n\t\t<PlatformToolset>{toolset}</PlatformToolset>",
            text,
        )

    vcxproj_path.write_text(text, encoding="utf-8")
    log(f"Patched {vcxproj_path} -> PlatformToolset {toolset}")


def patch_example_projects(addon_root: Path, toolset: str) -> None:
    patch_vcxproj_toolset(addon_root / "example_asr" / "example_asr.vcxproj", toolset)
    patch_vcxproj_toolset(addon_root / "example_tts" / "example_tts.vcxproj", toolset)


def install_onnxruntime(addon_root: Path) -> None:
    system, _uname_os, arch, win_arch = host_os_arch()

    libs_dir = addon_root / "libs"
    onnx_dir = libs_dir / "onnxruntime"
    libs_dir.mkdir(parents=True, exist_ok=True)

    compat_dest = None

    if system == "Linux" and arch == "x86_64":
        file_name = f"onnxruntime-linux-x64-{ONNX_VERSION}.tgz"
        dest = "linux64"
    elif system == "Linux" and arch == "arm64":
        file_name = f"onnxruntime-linux-aarch64-{ONNX_VERSION}.tgz"
        dest = "linuxaarch64"
        compat_dest = "linuxarm64"
    elif system == "Darwin" and arch == "arm64":
        file_name = f"onnxruntime-osx-arm64-{ONNX_VERSION}.tgz"
        dest = "macos"
    elif system == "Darwin" and arch == "x86_64":
        file_name = f"onnxruntime-osx-x86_64-{ONNX_VERSION}.tgz"
        dest = "macos"
    elif system == "Windows" and win_arch == "x64":
        file_name = f"onnxruntime-win-x64-{ONNX_VERSION}.zip"
        dest = "windows64"
    elif system == "Windows" and win_arch == "ARM64":
        file_name = f"onnxruntime-win-arm64-{ONNX_VERSION}.zip"
        dest = "windowsarm64"
    else:
        raise SystemExit(f"Unsupported platform: {system}/{arch}")

    url = f"https://github.com/microsoft/onnxruntime/releases/download/v{ONNX_VERSION}/{file_name}"

    log(f"Installing ONNX Runtime {ONNX_VERSION}")
    log(f"Platform: {system}/{arch}")
    log(f"Archive : {file_name}")

    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        archive = tmp / file_name

        download(url, archive)
        extract_archive(archive, tmp)

        extracted = next((p for p in tmp.glob("onnxruntime*") if p.is_dir()), None)
        if extracted is None:
            raise SystemExit("Could not find extracted ONNX Runtime directory.")

        if onnx_dir.exists():
            shutil.rmtree(onnx_dir)

        (onnx_dir / "lib" / dest).mkdir(parents=True, exist_ok=True)
        shutil.copytree(extracted / "include", onnx_dir / "include")

        suffixes = (".dylib", ".so", ".a", ".lib", ".dll")
        copy_matching(extracted / "lib", onnx_dir / "lib" / dest, suffixes)

        if system == "Linux":
            lib_dir = onnx_dir / "lib" / dest
            versioned = sorted(lib_dir.glob("libonnxruntime.so.*"))
            if versioned:
                for link_name in ["libonnxruntime.so", "libonnxruntime.so.1"]:
                    link = lib_dir / link_name
                    if not link.exists():
                        try:
                            link.symlink_to(versioned[0].name)
                        except OSError:
                            shutil.copy2(versioned[0], link)

        elif system == "Darwin":
            lib_dir = onnx_dir / "lib" / dest
            versioned = lib_dir / f"libonnxruntime.{ONNX_VERSION}.dylib"
            link = lib_dir / "libonnxruntime.dylib"

            if versioned.exists() and not link.exists():
                try:
                    link.symlink_to(versioned.name)
                except OSError:
                    shutil.copy2(versioned, link)

        if compat_dest:
            shutil.copytree(
                onnx_dir / "lib" / dest,
                onnx_dir / "lib" / compat_dest,
                dirs_exist_ok=True,
            )

        expected = [
            "libonnxruntime.so",
            "libonnxruntime.dylib",
            "onnxruntime.dll",
            "onnxruntime.lib",
        ]

        if not any((onnx_dir / "lib" / dest / n).exists() for n in expected):
            raise SystemExit(f"Failed to install ONNX Runtime library into {onnx_dir / 'lib' / dest}")

    log(f"Installed ONNX Runtime into {onnx_dir}")


def build_sherpa_onnx(addon_root: Path, vs_toolset: str = DEFAULT_WINDOWS_TOOLSET) -> None:
    require_tool("git")
    require_tool("cmake")

    system, uname_os, arch, win_arch = host_os_arch()

    install_dir = addon_root / "libs" / "sherpa-onnx"
    src_dir = addon_root / "sherpa-onnx-src"
    build_dir = src_dir / "build"
    platform_lib_dir = install_dir / "lib" / f"{uname_os}_{arch}"

    log("---")
    log(f"Building for OS: {uname_os} ({arch})")
    log(f"Installing to: {install_dir}")
    log("---")

    if src_dir.exists():
        log("--- Source directory already exists. Clearing old source to guarantee rebuild ---")
        shutil.rmtree(src_dir, ignore_errors=True)

    log("--- Cloning sherpa-onnx ---")
    run(["git", "clone", "https://github.com/k2-fsa/sherpa-onnx.git", str(src_dir)])

    build_dir.mkdir(parents=True, exist_ok=True)

    cmake_cmd = [
        "cmake",
        "..",
        "-DCMAKE_INSTALL_PREFIX=./install",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DSHERPA_ONNX_ENABLE_PYTHON=OFF",
        "-DSHERPA_ONNX_ENABLE_TESTS=OFF",
        "-DSHERPA_ONNX_ENABLE_BINARY=OFF",
        "-DSHERPA_ONNX_ENABLE_C_API=ON",
        "-DSHERPA_ONNX_USE_STATIC_CRT=OFF",
    ]

    if system == "Windows":
        cmake_cmd += [
            "-A",
            win_arch,
            "-T",
            vs_toolset,
            "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
            "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL",
        ]

    if system == "Darwin":
        cmake_cmd.append("-DCMAKE_OSX_DEPLOYMENT_TARGET=10.14")

    log("--- Configuring build (static library matched with openFrameworks CRT/toolset) ---")
    run(cmake_cmd, cwd=build_dir)

    log("--- Compiling ---")
    parallel = str(os.cpu_count() or 1)
    run(["cmake", "--build", ".", "--config", "Release", "--parallel", parallel], cwd=build_dir)

    log("--- Installing to temporary directory ---")
    run(["cmake", "--install", ".", "--config", "Release"], cwd=build_dir)

    log(f"--- Copying files to {install_dir} ---")

    if install_dir.exists():
        shutil.rmtree(install_dir, ignore_errors=True)

    (install_dir / "include").mkdir(parents=True, exist_ok=True)
    platform_lib_dir.mkdir(parents=True, exist_ok=True)

    include_src = build_dir / "install" / "include"

    if include_src.exists():
        shutil.copytree(include_src, install_dir / "include", dirs_exist_ok=True)

    suffixes = (".a", ".so", ".dylib", ".lib", ".dll")

    for lib_root in [
        build_dir / "install" / "lib",
        build_dir / "_deps" / "onnxruntime-src" / "lib",
    ]:
        copy_matching(lib_root, platform_lib_dir, suffixes)

    local_ort = addon_root / "libs" / "onnxruntime" / "lib"

    ort_dest_by_platform = {
        "Linux_x86_64": "linux64",
        "Linux_arm64": "linuxaarch64",
        "Darwin_arm64": "macos",
        "Darwin_x86_64": "macos",
        "Windows_x86_64": "windows64",
        "Windows_arm64": "windowsarm64",
    }

    local_dest = ort_dest_by_platform.get(f"{uname_os}_{arch}")

    if local_dest and (local_ort / local_dest).exists():
        shutil.copytree(local_ort / local_dest, platform_lib_dir, dirs_exist_ok=True)

    log("--- Cleaning up source folder ---")
    shutil.rmtree(src_dir, ignore_errors=True)

    log("Done!")
    log(f"Static libraries and headers are in {install_dir}")


def download_asr_models(addon_root: Path, language: str = "all") -> None:
    model_roots = [addon_root / example / "bin" / "data" / "models" for example in ASR_EXAMPLES]

    log("---")
    log("Target ASR model directories:")
    for models_dir in model_roots:
        log(f"  {models_dir}")
        models_dir.mkdir(parents=True, exist_ok=True)
    log("---")

    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)

        for model in ASR_MODELS:
            if language != "all" and model["lang"] != language:
                continue
            targets = [models_dir / model["dir"] for models_dir in model_roots]
            existing = next((target for target in targets if target.exists()), None)
            missing = [target for target in targets if not target.exists()]

            if not missing:
                log(f"--- Model directory '{model['dir']}' already exists in all ASR examples. Skipping download. ---")
                continue

            if existing is not None:
                log(f"--- Reusing existing model '{model['dir']}' from {existing} ---")
                for target in missing:
                    log(f"Copying to {target}")
                    shutil.copytree(existing, target)
                continue

            archive_name = Path(model["url"]).name
            archive = tmp / archive_name

            log(f"--- Downloading {model['label']}: {archive_name} ---")
            download(model["url"], archive)

            for target in missing:
                log(f"--- Extracting {archive_name} to {target} ---")
                target.mkdir(parents=True, exist_ok=True)
                extract_archive(archive, target, strip_components=1)
                log(f"Done: {target}")


def download_tts_models(addon_root: Path, language: str = "all") -> None:
    for model in TTS_MODELS:
        if language != "all" and model["lang"] != language:
            continue
        model_dir = addon_root / model["target"]
        model_dir.mkdir(parents=True, exist_ok=True)

        log(f"Downloading {model['label']}...")

        for local_name, remote_name in model["files"].items():
            target = model_dir / local_name

            if target.exists() and target.stat().st_size > 0:
                continue

            url = f"{model['base_url']}/{remote_name}"

            try:
                download(url, target)
            except Exception:
                if local_name in model["optional"]:
                    log(f"Optional file missing: {local_name}")
                else:
                    raise

        for local_name in model["files"]:
            if local_name in model["optional"]:
                continue

            target = model_dir / local_name

            if not target.exists() or target.stat().st_size == 0:
                raise SystemExit(f"Download failed or empty file: {target}")

        lexicon = model_dir / "lexicon.txt"

        if not lexicon.exists() or lexicon.stat().st_size == 0:
            log("Lexicon missing or empty; fetching espeak-ng-data for phonemizer...")
            require_tool("git")

            with tempfile.TemporaryDirectory() as td:
                tmp = Path(td)

                run(["git", "clone", "--depth", "1", model["clone_for_espeak"], str(tmp)])

                src = tmp / "espeak-ng-data"
                dst = model_dir / "espeak-ng-data"

                if dst.exists():
                    shutil.rmtree(dst)

                if src.exists():
                    shutil.move(str(src), str(dst))
                else:
                    log("Warning: espeak-ng-data not found in the repository.")

        log(f"{model['label']} download complete.")


def main() -> None:
    parser = argparse.ArgumentParser(description="Install/build ofxSherpaOnnx dependencies cross-platform.")

    parser.add_argument("--scripts-dir", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--skip-models", action="store_true")
    parser.add_argument(
        "--only",
        choices=["onnxruntime", "build", "asr-models", "tts-models"],
        help="Run only one installer step. Used by the Linux/macOS compatibility wrapper scripts.",
    )
    parser.add_argument("--asr-language", choices=["all", "en", "de"], default="all")
    parser.add_argument("--tts-language", choices=["all", "en", "de"], default="all")
    parser.add_argument(
        "--vs-toolset",
        default=os.environ.get("OF_VS_TOOLSET", DEFAULT_WINDOWS_TOOLSET),
        help="Windows Visual Studio toolset. Default: v145. Example: --vs-toolset v143",
    )

    args = parser.parse_args()

    scripts_dir = args.scripts_dir.resolve()
    addon_root = scripts_dir.parent

    log("ofxSherpaOnnx one-shot installer")
    log(f"Addon root: {addon_root}")
    log("")

    system, *_ = host_os_arch()

    if args.only == "onnxruntime":
        log("==> Install ONNX Runtime")
        install_onnxruntime(addon_root)
        log("\nSelected setup step completed.")
        return

    if args.only == "build":
        require_tool("cmake")
        require_tool("git")
        log("==> Build sherpa-onnx")
        build_sherpa_onnx(addon_root, vs_toolset=args.vs_toolset)
        if system == "Windows":
            log("\n==> Patch Visual Studio example projects")
            patch_example_projects(addon_root, args.vs_toolset)
        log("\nSelected setup step completed.")
        return

    if args.only == "asr-models":
        log("==> Download ASR models")
        download_asr_models(addon_root, language=args.asr_language)
        log("\nSelected setup step completed.")
        return

    if args.only == "tts-models":
        require_tool("git")
        log("==> Download TTS models")
        download_tts_models(addon_root, language=args.tts_language)
        log("\nSelected setup step completed.")
        return

    require_tool("cmake")
    require_tool("git")

    log("==> Install ONNX Runtime")
    install_onnxruntime(addon_root)

    if not args.skip_build:
        log("\n==> Build sherpa-onnx")
        build_sherpa_onnx(addon_root, vs_toolset=args.vs_toolset)

    if system == "Windows":
        log("\n==> Patch Visual Studio example projects")
        patch_example_projects(addon_root, args.vs_toolset)

    if not args.skip_models:
        log("\n==> Download ASR models")
        download_asr_models(addon_root, language=args.asr_language)

        log("\n==> Download TTS models")
        download_tts_models(addon_root, language=args.tts_language)

    log("\nAll setup steps completed.")


if __name__ == "__main__":
    main()