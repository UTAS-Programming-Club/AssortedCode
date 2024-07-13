#! /bin/sh

# TODO: Only redownload pandoc if a flag is given

if [ $# -ne 2 ] ; then
  printf "%s zip_path wiki_git_url\n" "$0"
  exit 1
fi

INPUT_PATH=input
OUTPUT_PATH=wiki
SIDEBAR_FILE=_Sidebar.md

wget -q https://github.com/jgm/pandoc/releases/download/3.2.1/pandoc-3.2.1-linux-amd64.tar.gz -O pandoc.tar.gz
tar --strip-components 1 -xf pandoc.tar.gz pandoc-3.2.1/bin/pandoc
rm pandoc.tar.gz

rm -r $INPUT_PATH 2> /dev/null
mkdir $INPUT_PATH
unzip -q "$1" -d $INPUT_PATH

if [ ! -d "$OUTPUT_PATH" ]; then
  git clone -q "$2" "$OUTPUT_PATH"
fi

printf "" > "$OUTPUT_PATH/$SIDEBAR_FILE"

cd "$INPUT_PATH" || exit 1
find . -exec sh -c '
  INPUT_PATH="${3#./}"
  NO_EXT_INPUT_PATH="${INPUT_PATH%.typ}"
  OUTPUT_FILE="$(echo "$NO_EXT_INPUT_PATH" | sed "s#/#: #g").md"
  OUTPUT_PATH="$2/$OUTPUT_FILE"

  if [ "$INPUT_PATH" != . ]; then
    printf "  %*s* [%s](%s/wiki/%s)\n" \
      "$((2 * $(echo "$INPUT_PATH" | tr -cd '/' | wc -c)))" "" \
      "$(basename "$NO_EXT_INPUT_PATH")" \
      "${4%.wiki.git}" \
      "$(echo "$NO_EXT_INPUT_PATH" | sed -e "s#/#%3A #g" -e "s/ /-/g")" >> "../$2/$5"
  fi
  if [ ! -f "$3" ]; then
    exit 0
  fi

  echo "$1/$INPUT_PATH -> $OUTPUT_PATH"

  ../bin/pandoc -f typst -t gfm --wrap=preserve "$3" -o "../$OUTPUT_PATH"
  git -C "../$2" add "$OUTPUT_FILE"
  ' sh "$INPUT_PATH" "$OUTPUT_PATH" "{}" "$2" "$SIDEBAR_FILE" \;

cd "../$OUTPUT_PATH" || exit 1

git add "$SIDEBAR_FILE"
git commit -m "Typst Export $(date -u +%Y-%m-%dT%H:%M:%SZ)"
git push
