#!/usr/bin/python

from subprocess import check_output
from subprocess import CalledProcessError
from Queue import Queue
import concurrent.futures
import re
import sys

SCHEME = 'LiquidCoreiOS'
WORKSPACE = 'LiquidCoreiOS.xcodeproj/project.xcworkspace'
TEST_SCHEME = 'LiquidCoreiOSTests'

def get_test_list(prefix = 'test'):
    args = [
        "xctool",
        "-workspace", WORKSPACE,
        "-scheme", SCHEME,
        "-sdk", "iphonesimulator",
        "run-tests", "-listTestsOnly"
    ]
    raw = check_output(args)

    return re.findall(prefix + '[A-Za-z0-9_]*', raw)

q = Queue()

def run_test(test):
    test_scheme = TEST_SCHEME + ':' + TEST_SCHEME + '/' + test
    args = [
        "xctool",
        "-workspace", WORKSPACE,
        "-scheme", SCHEME,
        "-sdk", "iphonesimulator",
        "run-tests", "-only", test_scheme
    ]
    success = 1
    try:
        raw = check_output(args)
    except CalledProcessError as e:
        raw = e.output
        success = 0
    for line in raw.split("\n"):
        if "[" + TEST_SCHEME + " " + test + "]" in line and line.strip()[0] != '-':
            sys.stdout.write(line + '\n')
            break
    q.put(success)

def run_tests(prefix):
    tests = get_test_list(prefix)

    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as executor:
        {executor.submit(run_test, test): test for test in tests}

    total_tests = 0
    successful_tests = 0

    while not q.empty():
        total_tests += 1
        successful_tests += q.get()    

    failed_tests = total_tests - successful_tests
    p = int(100*successful_tests / total_tests)
    print 'DONE: ' + str(successful_tests) + ' of ' + str(total_tests) + ' passed (' + str(p) + '%), ' + str(failed_tests) + ' failed.'

def main():
    if (len(sys.argv) > 1):
        run_tests(sys.argv[1])
    else:
        run_tests('testCcPassing')

if __name__ == '__main__':
    main()