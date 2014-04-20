#!/bin/bash

echo "inside $0"

if [ -n "$LOCALE_OVERRIDE" ]; then
    export LC_ALL="$LOCALE_OVERRIDE";
    echo "Setting LC_ALL to $LOCALE_OVERRIDE"
    curdir="$(pwd)"
    cd /tmp
    pycmd='import pandas; print("pandas detected console encoding: %s" % pandas.get_option("display.encoding"))'
    python -c "$pycmd"
    cd "$curdir"
fi

# conditionally build and upload docs to GH/pandas-docs/pandas-docs/travis
"$TRAVIS_BUILD_DIR"/ci/build_docs.sh 2>&1 > /tmp/doc.log &
# doc build log will be shown after tests

if [[ "$COVERAGE" == "true" ]]; then
    export WITH_COVERAGE="--with-coverage"
else
    export WITH_COVERAGE=""
fi
nosetests -s -v $WITH_COVERAGE sklearn

echo nosetests --exe -w /tmp -A "$NOSE_ARGS" pandas --with-xunit --xunit-file=/tmp/nosetests.xml
nosetests --exe -w /tmp -A "$NOSE_ARGS" $WITH_COVERAGE pandas --with-xunit --xunit-file=/tmp/nosetests.xml

RET="$?"

# wait until subprocesses finish (build_docs.sh)
wait

exit "$RET"
