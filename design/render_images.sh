#!/usr/bin/env bash
# Renders PNG pictures of every part in techcardadapter.scad (needs OpenSCAD; on a
# headless machine run it through xvfb-run).  Usage:  ./render_images.sh
# The .scad file draws one part at the top level (plate();), so we make a temporary
# copy without that call and append the call for the part we want.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p images
SRC=techcardadapter.scad
# module name : output name (= the STL name)
PARTS="plate:bottomplate smallplate:smallplate rotor:rotor smallrotor:smallrotor"
for p in $PARTS; do
  mod=${p%%:*}; out=${p##*:}
  tmp=$(mktemp --suffix=.scad)
  grep -v -E '^[[:space:]]*(//)?[[:space:]]*(plate|smallplate|smallrotor|rotor)\(\);' "$SRC" > "$tmp"
  echo "${mod}();" >> "$tmp"
  # isometric view
  openscad -o "images/${out}_iso.png" --imgsize=900,700 --autocenter --viewall \
           --camera=0,0,0,55,0,25,0 --render --colorscheme=Tomorrow --projection=p "$tmp"
  # top view
  openscad -o "images/${out}_top.png" --imgsize=900,700 --autocenter --viewall \
           --camera=0,0,0,0,0,0,0 --render --colorscheme=Tomorrow --projection=o "$tmp"
  rm -f "$tmp"
done
