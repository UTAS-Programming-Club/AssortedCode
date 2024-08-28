#! /bin/sh

# TODO: Only redownload pandoc if a flag is given

if [ $# -ne 3 ]; then
  printf "%s zip_path git_url image_git_branch\n" "$0"
  exit 1
fi

INPUT_PATH=input
OUTPUT_WIKI_PATH=repos/wiki
OUTPUT_IMAGES_PATH=repos/images
SIDEBAR_FILE=_Sidebar.md

wget -q https://github.com/jgm/pandoc/releases/download/3.3/pandoc-3.3-linux-amd64.tar.gz -O pandoc.tar.gz
tar --strip-components 1 -xf pandoc.tar.gz pandoc-3.3/bin/pandoc
rm pandoc.tar.gz

rm -r "$INPUT_PATH" 2> /dev/null
mkdir "$INPUT_PATH"
unzip -q "$1" -d "$INPUT_PATH"

if [ ! -d "$OUTPUT_IMAGES_PATH" ]; then
  git clone -q "$2" "$OUTPUT_IMAGES_PATH"
fi
git -C "$OUTPUT_IMAGES_PATH" pull -q 2>/dev/null

IMAGES_REPO_BRANCH=$(git -C "$OUTPUT_IMAGES_PATH" branch --show-current)
if [ "$IMAGES_REPO_BRANCH" != "$3" ]; then
  git -C "$OUTPUT_IMAGES_PATH" switch -q "$3" 2>/dev/null || { 
    git -C "$OUTPUT_IMAGES_PATH" checkout -q --orphan "$3"
    git -C "$OUTPUT_IMAGES_PATH" rm -rfq .
  }
fi

if [ ! -d "$OUTPUT_WIKI_PATH" ]; then
  git clone -q "${2%.git}.wiki.git" "$OUTPUT_WIKI_PATH"
fi
git -C "$OUTPUT_WIKI_PATH" pull -q

printf "" > "$OUTPUT_WIKI_PATH/$SIDEBAR_FILE"

cd "$INPUT_PATH" || exit 1
find . -exec sh -c '
  INPUT_PATH="${6#./}"
  NO_EXT_INPUT_PATH="${INPUT_PATH%.typ}"
  DIR_COUNT="$((2 * $(echo "$INPUT_PATH" | tr -cd '/' | wc -c)))"
  FILE="$(basename "$NO_EXT_INPUT_PATH")"

  if [ -f "$6" ]; then
    # echo "$4 -> $(dirname "$3")"
    case $INPUT_PATH in
      *.typ)
        printf "  %*s* [%s](%s/wiki/%s)\n" \
          "$DIR_COUNT" "" \
          "$FILE" \
          "${4%.git}" \
          "$(echo "$NO_EXT_INPUT_PATH" | sed -e "s#/#%3A #g" -e "s/ /-/g")" >> "../$3/$5"
        OUTPUT_PATH="$3/$(echo "$NO_EXT_INPUT_PATH" | sed "s#/#: #g").md"
        sed -i.bak "s@#image(\"..@#image(\"https://raw.githubusercontent.com/UTAS-Programming-Club/UntitledTextAdventure/images@" "$6"
        ../bin/pandoc -f typst -t gfm --wrap=preserve "$6" -o "../$OUTPUT_PATH"
        ;;
      *.png)
        OUTPUT_PATH="$2/$INPUT_PATH"
        mkdir -p "$(dirname ../"$OUTPUT_PATH")"
        cp "$INPUT_PATH" "../$OUTPUT_PATH"
    esac
  elif [ "$INPUT_PATH" != . ] && [ "$INPUT_PATH" != Images ]; then
    printf "  %*s* %s\n" \
      "$DIR_COUNT" "" \
      "$FILE"  >> "../$3/$5"
  fi

  if [ ! -f "$6" ]; then
    exit 0
  fi

  echo "$1/$INPUT_PATH -> $OUTPUT_PATH"
' sh "$INPUT_PATH" "$OUTPUT_IMAGES_PATH" "$OUTPUT_WIKI_PATH" "$2" "$SIDEBAR_FILE" "{}" \;

COMMIT_NAME="Typst Export $(date -u +%Y-%m-%dT%H:%M:%SZ)"

(
cd "../$OUTPUT_IMAGES_PATH" || exit 1
git add .
git commit -m "$COMMIT_NAME"
git push || git push --set-upstream origin "$3"
)

cd "../$OUTPUT_WIKI_PATH" || exit 1

git add .
git commit -m "$COMMIT_NAME"
git push
