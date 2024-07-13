#! /bin/bash

# TODO: Only redownload pandoc if a flag is given

if [ $# -ne 2 ] ; then
  printf "%s zip_path wiki_git_url\n" "$0"
  exit 1
fi

INPUT_PATH=input
OUTPUT_PATH=wiki

wget -q https://github.com/jgm/pandoc/releases/download/3.2.1/pandoc-3.2.1-linux-amd64.tar.gz -O pandoc.tar.gz
tar --strip-components 1 -xf pandoc.tar.gz pandoc-3.2.1/bin/pandoc
rm pandoc.tar.gz

rm -r $INPUT_PATH 2> /dev/null
mkdir $INPUT_PATH
unzip -q "$1" -d $INPUT_PATH

if [ ! -d "$OUTPUT_PATH" ]; then
  git clone -q "$2" "$OUTPUT_PATH"
fi

cd "$INPUT_PATH" || exit 1
find . -type f -exec sh -c '
  FILE="${3#./}"
  OUTPUT_FILE="${FILE%.typ}.md"
  OUTPUT_PATH="$2/$OUTPUT_FILE"
  echo "$1/$FILE -> $OUTPUT_PATH"
  mkdir "../$2/$(dirname "$3")" 2> /dev/null
  ../bin/pandoc -f typst -t gfm --wrap=preserve "$3" -o "../$OUTPUT_PATH"
  git -C "../$2" add "$OUTPUT_FILE"
  ' sh "$INPUT_PATH" "$OUTPUT_PATH" "{}" \;

cd "../$OUTPUT_PATH" || exit 1
git commit -m "Typst Export $(date -u +%Y-%m-%dT%H:%M:%SZ)"
git push
