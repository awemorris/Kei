#!/bin/sh
# ws075-p001: what the i915 executor's shader compiler does not take, over every shader of today's desktop and graphics.
#  1. The shaders: the Vulkan clients' own (glslc), libGL's fixed function and the GLSL host tests' programs (as
#     plan/ws068/tests/glsl-host/run.sh linked them into build/ws068-glsl-host; run that first), and the programs of
#     egltest's and glxtest's scenes (extract.py, pair.py: zedBSD's GLSL compiler links them as libGLESv2 does; the
#     vertex stage gets libGLESv2's gl_Position rewrite through spirv-test).
#  2. survey.py over each: every gap, in OUTDIR/modules.txt; how many modules each gap stops, in OUTDIR/gaps.txt.
#  3. The i915 compiler itself (plan/ws068/tests/i915-shader-check) over each vertex and fragment module, next to the
#     survey's first gap, in OUTDIR/first.txt: a module the compiler refuses that the survey calls ok has a gap the
#     survey does not copy (a shape rule).
#
#   plan/ws075/tests/shader-survey/run.sh [OUTDIR]      (default build/ws075-p001/shaders)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
here=$(cd "$(dirname -- "$0")" && pwd)
root=$here/../../../..
out=${1:-$root/build/ws075-p001/shaders}
host=$root/build/ws068-glsl-host
rm -rf "$out"
mkdir -p "$out/apps" "$out/scenes" "$out/programs" "$out/shim"

# The tools: zedBSD's GLSL compiler's driver, libGLESv2's gl_Position rewrite, the i915 compiler on the host.
for h in EGL GLES2 GLES3 KHR; do ln -sfn "$root/include/libc/$h" "$out/shim/$h"; done
ln -sfn "$root/userland/desktop/keiland/wayland-egl-core.h" "$out/shim/wayland-egl-core.h"
cc -std=c11 -O1 -w -o "$out/glsl-test" "$root/plan/ws068/tests/glsl-host/glsl-test.c" "$root"/userland/desktop/libglesv2/glsl/*.c -lm || exit 1
cc -std=c99 -w -I"$out/shim" -o "$out/spirv-test" "$root/plan/ws068/tests/spirv-host/main.c" "$root/userland/desktop/libglesv2/spirv.c" || exit 1
cc -std=gnu99 -O0 -w -I"$root" -I"$root/include" -DHAL_ARCH_AMD64 -o "$out/i915-check" \
	"$root/plan/ws068/tests/i915-shader-check/main.c" -lm || exit 1

# The Vulkan clients' shaders.
for f in "$root"/userland/desktop/wayland/shaders/*.[fv]*[gt] "$root"/userland/desktop/files/shaders/*.[fv]*[gt] \
	"$root"/userland/desktop/terminal/shaders/*.[fv]*[gt] "$root"/userland/tests/mview/shaders/*.[fv]*[gt] "$root"/userland/tests/vkdemo/shaders/*.[fv]*[gt]; do
	name=$(echo "$f" | sed 's|.*/userland/base/||; s|/shaders/|-|')
	# As the client is built: its checked-in SPIR-V, else glslc with the flags its regenerate.py gives (-O inlines calls).
	if [ -f "$f.spv" ]; then
		cp "$f.spv" "$out/apps/$name.spv"
		continue
	fi
	optimize=
	grep -q '"-O"' "$(dirname -- "$f")/regenerate.py" 2>/dev/null && optimize=-O
	glslc --target-env=vulkan1.0 $optimize -o "$out/apps/$name.spv" "$f" || echo "glslc failed: $f"
done

# The GLSL host tests' programs (vertex stages rewritten) and libGL's fixed function.
for f in "$host"/*.linked.spv "$host"/*.geom.spv; do
	[ -f "$f" ] && cp "$f" "$out/programs/"
done

# egltest's and glxtest's scenes, linked as libGLESv2 links them.
python3 "$here/extract.py" "$out/scenes/src" "$root"/userland/tests/egltest/*.c "$root"/userland/retro/glxtest/*.c > "$out/extracted.txt"
python3 "$here/pair.py" "$out/glsl-test" "$out/scenes" $(cat "$out/extracted.txt") > "$out/unpaired.txt"
for f in "$out"/scenes/*.vert.spv; do
	"$out/spirv-test" "$f" "${f%.spv}.linked.spv" > /dev/null 2>&1 && mv "${f%.spv}.linked.spv" "$f"
done

# The survey, and the gaps counted by module.
modules=$(ls "$out"/apps/*.spv "$out"/programs/*.spv "$out"/scenes/*.spv)
python3 "$here/survey.py" $modules | sed "s|$out/||" > "$out/modules.txt"
sed 's/^[^:]*: //' "$out/modules.txt" | tr ';' '\n' | sed 's/^ *//' | grep -v '^ok$' | sort | uniq -c | sort -rn > "$out/gaps.txt"

# The compiler's own first refusal next to the survey's first gap.
: > "$out/first.txt"
for f in $modules; do
	case "$f" in
	*.vert.spv|*.vert.linked.spv|*-*.vert.spv) stage=vertex ;;
	*.frag.spv|*.frag.linked.spv) stage=fragment ;;
	*) continue ;;
	esac
	compiler=$("$out/i915-check" "$stage" "$f" 2>&1 | sed 's/^[^:]*: //' | head -1)
	survey=$(python3 "$here/survey.py" --first "$f" | sed 's/^[^:]*: //')
	echo "$(basename "$f"): compiler: $compiler | survey: $survey" >> "$out/first.txt"
done
echo "shader-survey: $(echo $modules | wc -w) modules, $(grep -vc ': ok$' "$out/modules.txt") with gaps; $out/gaps.txt"
