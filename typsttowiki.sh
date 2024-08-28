#! /bin/sh

# TODO: Only redownload pandoc if a flag is given
# TODO: Reverse sort on dated session files to put newest at top?

if [ $# -ne 3 ]; then
  printf "%s zip_path git_url image_git_branch\n" "$0"
  exit 1
fi

INPUT_PATH=input
OUTPUT_IMAGES_PATH=repos/images
OUTPUT_WIKI_PATH=repos/wiki
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

# shellcheck disable=SC2016
find . -print0 | sort -z | xargs -0 sh -c '
INPUTS_PATH="$1"
OUTPUT_IMAGES_PATH="$2"
OUTPUT_WIKI_PATH="$3"
SIDEBAR_FILE="$4"
GIT_URL="$5"
IMAGES_GIT_BRANCH="$6"

shift 6
for FILE; do
  INPUT_PATH="${FILE#./}"
  NO_EXT_INPUT_PATH="${INPUT_PATH%.typ}"
  DIR_COUNT="$((2 * $(echo "$INPUT_PATH" | tr -cd '/' | wc -c)))"
  BASE_FILE="$(basename "$NO_EXT_INPUT_PATH")"

  if [ -f "$FILE" ]; then
    case $INPUT_PATH in
      *.typ)
        printf "  %*s* [%s](%s/wiki/%s)\n" \
          "$DIR_COUNT" "" \
          "$BASE_FILE" \
          "${GIT_URL%.git}" \
          "$(echo "$NO_EXT_INPUT_PATH" | sed -e "s#/#%3A #g" -e "s/ /-/g")" >> "../$OUTPUT_WIKI_PATH/$SIDEBAR_FILE"
        OUTPUT_PATH="$OUTPUT_WIKI_PATH/$(echo "$NO_EXT_INPUT_PATH" | sed "s#/#: #g").md"
        sed -i.bak "s@#image(\"\(\.\./\)\+@#image(\"https://raw.githubusercontent${GIT_URL#https://github}/$IMAGES_GIT_BRANCH/@" "$FILE"
        ../bin/pandoc -f typst -t gfm --wrap=preserve "$FILE" -o "../$OUTPUT_PATH"
        ;;
      *.png)
        OUTPUT_PATH="$OUTPUT_IMAGES_PATH/$INPUT_PATH"
        mkdir -p "$(dirname ../"$OUTPUT_PATH")"
        cp "$INPUT_PATH" "../$OUTPUT_PATH"
    esac
    echo "$INPUTS_PATH/$INPUT_PATH -> $OUTPUT_PATH"
  elif [ "$INPUT_PATH" != . ] && [ "$INPUT_PATH" != Images ]; then
    printf "  %*s* %s\n" \
      "$DIR_COUNT" "" \
      "$BASE_FILE"  >> "../$OUTPUT_WIKI_PATH/$SIDEBAR_FILE"
  fi
done' sh "$INPUT_PATH" "$OUTPUT_IMAGES_PATH" "$OUTPUT_WIKI_PATH" "$SIDEBAR_FILE" "$2" "$3"

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
