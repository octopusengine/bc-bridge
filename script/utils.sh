# vim: set ts=4:

: ${BUILD_DIR:="$(pwd)/build"}


die() {
	# bold red
	printf '\033[1;31mERROR:\033[0m %s\n' "$1" >&2
	shift
	printf '  %s\n' "$@"
	exit 2
}

einfo() {
	# bold cyan
	printf '\n\033[1;36m> %s\033[0m\n' "$@" >&2
}

# Prints the given arguments and runs them.
run() {
	printf '$ %s\n' "$*"
	"$@"
}

# Fetches the given URL and verifies SHA256 checksum.
wgets() {(
	local url="$1"
	local sha256="$2"
	local dest="${3:-.}"

	mkdir -p "$dest" \
		&& cd "$dest" \
		&& rm -f "${url##*/}" \
		&& wget -T 10 "$url" \
		&& echo "$sha256  ${url##*/}" | sha256sum -c
)}
