#!/bin/sh
set -eu
package_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$package_dir/bin/pocket-engineer-server" --open "$package_dir/share/pocket-engineer/www"
