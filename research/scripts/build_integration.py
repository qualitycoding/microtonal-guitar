#!/usr/bin/env python3
"""Resumable g++ builder for the MicroGuitar integration-test harness (S-ADD-004).

The sandbox recycles within ~15 min and wipes /tmp, and the durable mount is noexec with
flaky writes. So: compile each unit to a .o on the mount (single tmp-write + rename), skip
units already cached, and link/run in /tmp at the end. Re-invoke after every recycle.

Usage:
  python3 build_integration.py [--repo DIR] [--juce DIR] [--src-root DIR] compile N
  python3 build_integration.py [--repo DIR] [--juce DIR] [--src-root DIR] link OUT
  python3 build_integration.py [--repo DIR] [--juce DIR] [--src-root DIR] list

--src-root: directory whose tests/integration is used (the remediation branch or a baseline
worktree for red runs). Defaults to the repo itself.
"""
import os, shlex, subprocess, sys, time

ROOT = os.path.dirname(os.path.abspath(__file__))
CACHE = os.path.join(ROOT, "..", "..", "..", "obj-cache")
CACHE = os.path.normpath(os.path.join(ROOT, "..", "..", "..", "obj-cache"))

COMMON_DEFS = [
    "-DLINUX=1", "-DNDEBUG=1", "-D_NDEBUG=1",
    "-I/usr/include/freetype2",
    "-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1",
    "-DJUCE_STANDALONE_APPLICATION=1",
    "-DJUCE_USE_CURL=0", "-DJUCE_WEB_BROWSER=0",
    "-DJUCE_USE_XRANDR=0", "-DJUCE_USE_XINERAMA=0", "-DJUCE_USE_XCURSOR=0",
    "-DJUCE_USE_XINPUT=0", "-DJUCE_ALSA=0", "-DJUCE_JACK=0",
    "-DJucePlugin_Build_VST3=0", "-DJucePlugin_Build_VST2=0",
]

MODULES = [
    "juce_core", "juce_data_structures", "juce_events", "juce_graphics",
    "juce_gui_basics", "juce_gui_basics_2", "juce_gui_basics_3",
    "juce_gui_basics_4", "juce_gui_basics_5",
    "juce_gui_extra", "juce_audio_basics",
    "juce_audio_processors_headless", "juce_audio_processors",
]

def units(repo, juce, srcroot):
    u = []
    avail = " ".join(f"-DJUCE_MODULE_AVAILABLE_{m}=1" for m in MODULES)
    for m in MODULES:
        base = m.rsplit ("_", 1)[0] if m.rsplit ("_", 1)[-1].isdigit() else m
        u.append((m, [os.path.join(juce, "modules", base, m + ".cpp")],
                  COMMON_DEFS + [avail, "-I" + os.path.join(juce, "modules")]))
    # Extra implementation units that official JUCE CMake picks up via the module-root glob
    # but that are easy to miss: the build-timestamp unit and the plutovg C amalgamation.
    u.append(("juce_core_CompilationTime",
              [os.path.join(juce, "modules", "juce_core", "juce_core_CompilationTime.cpp")],
              COMMON_DEFS + [avail, "-I" + os.path.join(juce, "modules")]))
    u.append(("juce_graphics_Harfbuzz",
              [os.path.join(juce, "modules", "juce_graphics", "juce_graphics_Harfbuzz.cpp")],
              COMMON_DEFS + [avail, "-I" + juce + "/modules",
                             "-I" + os.path.join(juce, "modules", "juce_graphics", "fonts", "harfbuzz")]))
    u.append(("juce_graphics_Sheenbidi",
              [os.path.join(juce, "modules", "juce_graphics", "juce_graphics_Sheenbidi.c")],
              ["-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1", "-I" + os.path.join(juce, "modules"),
               "-I" + os.path.join(juce, "modules", "juce_graphics", "fonts", "sheenbidi")]))
    for w in ["libjpg_1","libjpg_2","libjpg_3","libpng","libwebp_dec","libwebp_dsp_dec",
              "libwebp_dsp_enc","libwebp_enc","libwebp_sharpyuv","libwebp_utils"]:
        u.append(("juce_graphics_"+w,
                  [os.path.join(juce, "modules", "juce_graphics", "juce_graphics_"+w+".c")],
                  ["-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1", "-I" + os.path.join(juce, "modules"),
                   "-I" + os.path.join(juce, "modules", "juce_graphics")]))
    for w,flags in [("libwebp_dsp_dec_sse41",["-msse4.1"]),("libwebp_dsp_dec_avx2",["-mavx2"]),
                    ("libwebp_dsp_enc_sse41",["-msse4.1"]),("libwebp_dsp_enc_avx2",["-mavx2"])]:
        u.append(("juce_graphics_"+w,
                  [os.path.join(juce, "modules", "juce_graphics", "juce_graphics_"+w+".c")],
                  ["-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1", "-I" + os.path.join(juce, "modules"),
                   "-I" + os.path.join(juce, "modules", "juce_graphics")] + flags))
    u.append(("juce_graphics_lunasvg",
              [os.path.join(juce, "modules", "juce_graphics", "juce_graphics_lunasvg.c")],
              ["-I" + os.path.join(juce, "modules"),
               "-I" + os.path.join(juce, "modules", "juce_graphics", "drawables", "lunasvg", "include"),
               "-I" + os.path.join(juce, "modules", "juce_graphics", "drawables", "lunasvg", "plutovg", "include"),
               "-I" + os.path.join(juce, "modules", "juce_graphics", "drawables", "lunasvg", "plutovg", "source"),
               "-I" + os.path.join(juce, "modules", "juce_graphics", "drawables", "lunasvg", "source")]))
    for name in ["PluginProcessor.cpp", "TuningEngine.cpp", "MicrotonalVoice.cpp",
                 "NoteNaming.cpp", "Instruments.cpp", "PluginEditor.cpp"]:
        u.append((name[:-4], [os.path.join(repo, name)],
                  COMMON_DEFS + [avail, "-I" + juce + "/modules", "-I" + repo, "-I" + os.path.join(repo, "tests", "integration")]))
    for name in ["TuningParameterTests.cpp", "InstrumentTests.cpp",
                 "VoiceGuardAndPitchTests.cpp", "LifecycleTests.cpp", "RunIntegrationTests.cpp"]:
        u.append((name[:-4], [os.path.join(srcroot, "tests", "integration", name)],
                  COMMON_DEFS + [avail, "-I" + juce + "/modules", "-I" + repo, "-I" + os.path.join(srcroot, "tests", "integration")]))
    return u

def compile_units(repo, juce, srcroot, n):
    os.makedirs(CACHE, exist_ok=True)
    done = missing = 0
    for name, srcs, defs in units(repo, juce, srcroot):
        obj = os.path.join(CACHE, name + ".o")
        if os.path.exists(obj) and os.path.getsize(obj) > 0:
            done += 1
            continue
        if n <= 0:
            missing += 1
            continue
        tmp = os.path.join("/tmp", "objmirror")
        os.makedirs(tmp, exist_ok=True)
        tmpobj = os.path.join(tmp, name + ".o")
        cc = "gcc" if srcs[0].endswith(".c") else "g++"
        std = [] if srcs[0].endswith(".c") else ["-std=gnu++17"]
        cmd = [cc] + std + ["-O2", "-w", "-c"] + defs + srcs + ["-o", tmpobj]
        print(f"[build {name}] ...", flush=True)
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            print(r.stderr[-3000:]); sys.exit(2)
        # copy to the durable cache with retries (FUSE writes can fail transiently)
        for attempt in range(5):
            try:
                with open(tmpobj, "rb") as fsrc, open(obj + ".tmp", "wb") as fdst:
                    fdst.write(fsrc.read())
                os.replace(obj + ".tmp", obj)
                break
            except OSError as e:
                print(f"copy retry {attempt}: {e}", flush=True)
                time.sleep(2)
        else:
            print("could not cache", obj); sys.exit(3)
        done += 1; n -= 1
    print(f"cached+built: {done}, still missing: {missing}")
    sys.exit(0 if missing == 0 else 1)

def link(out, repo):
    objs = sorted(os.path.join(CACHE, f) for f in os.listdir(CACHE) if f.endswith(".o"))
    cmd = ["g++", "-O2"] + objs + ["-o", out,
        "-lX11", "-lXext", "-lfreetype", "-lfontconfig", "-lpthread", "-ldl", "-lrt"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr[-4000:]); sys.exit(2)
    print("linked:", out)

if __name__ == "__main__":
    args = sys.argv[1:]
    repo, juce, srcroot = "/mnt/agents/repo", "/tmp/JUCE", "/mnt/agents/repo"
    while args and args[0].startswith("--"):
        k, v = args[0], args[1]; args = args[2:]
        if k == "--repo": repo = v
        elif k == "--juce": juce = v
        elif k == "--src-root": srcroot = v
    mode = args[0]
    if mode == "compile": compile_units(repo, juce, srcroot, int(args[1]))
    elif mode == "link": link(args[1], repo)
    elif mode == "list":
        for name, srcs, defs in units(repo, juce, srcroot): print(name, *srcs)
