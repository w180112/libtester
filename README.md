# libtester project

[![libtester ci](https://github.com/w180112/libtester/actions/workflows/ci.yml/badge.svg)](https://github.com/w180112/libtester/actions/workflows/ci.yml)

## Purpose
- libtester is a custom shell executor for ci and regular test.
- In some projects, which replies on sepcific execution environment to run custom regular ci or end2end test. Therefore we need a custom shell executor to make these projects test be run in specific environment.
- The libtester is encapsulated in a library, libtester. User can just call the libtester API to start the test.
- User should add a curl cmd to send test request to the tester.

## Usage
- Build:
    ```
    git clone https://github.com/w180112/libtester.git
    cd libtester && ./build.sh
    ```
    - build and install binary files
    ```
    make # build binaries
    make install # install related files to system
    ```
    - run unit tests
    ```
    make # build binaries
    make test
    ```
    - clean all binaries
    ```
    make clean # remove built binaries
    make uninstall # remove files installed in system
    ```
- Send a curl cmd to start test, e.g. add a curl cmd in github action file
    ```
    ci_test:
        stage: ci_test
        extends: .common
        timeout: 60 minutes
        script: |
            while (( 1 )); do
                status_code=$(curl --write-out %{http_code} --silent --output /dev/null -I -m 1200 -H "branch: "$GITHUB_REF_NAME"" -H "test_type: go-docx-replacer" 192.168.10.151:55688)
                echo "$status_code"
                case "$status_code" in
                    200)
                        echo "test successfully"
                        exit 0
                        ;;
                    503)
                        echo "previous test is still running, wait for 10 mins..."
                        sleep 600
                        ;;
                    400)
                        echo "request header error"
                        exit 10
                        ;;
                    500)
                        echo "test failed"
                        exit 10
                        ;;
                    *)
                        echo "unknown http code respond"
                        exit 10
                        ;;
                esac
            done
    ```
- User should add test type(e.g. test project name) and test branch(e.g. master, dev, ...etc) as test parameters in curl http header like above example. The default parameters are ***type1*** and ***master***.

- If client(e.g. github action) receives http code:
    - 200, then it means the test is passed.
    - 500, test is failed.
    - 400, test request parameters error.
    - 503, there is another same type test is still running.
