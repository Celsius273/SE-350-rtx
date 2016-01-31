SE350
=====

University of Waterloo SE350 Operating Systems RTX Project Starter Files and Documentation

## User test processes
There are 3 user test processes, the minimum required to test all features.
This is within the requirement of 2-6 processes and it outputs the correct format, except without initially printing the number of tests.
The tests were designed with debuggability in mind:
- The tests are easy to read.
  Each test says what it's evaluating, what process it starts from, and what process it ends at.
- The tests have very little boilerplate.
  Boilerplate is more code that can have bugs that a simpler test harness can prevent.
- The test output is easy to interpret.
  Specifically, any failures output the name and line of the failed test.
  Unfortunately, to output "total n tests" at the start, it's necessary to hard-code the number of tests.
  This is because the number of tests is unknown at the start of the user tests.
- The tests are comprehensive.
  This makes it easier to resolve ambiguity in the specification and verify that behaviour doesn't change over time.
  In particular, the tests cover some edge cases for process priority, where breaking changes could result in deadlocks.
- The tests are deterministic, assuming the operating system is deterministic.
  This makes test results reproducible.
  Unfortunately, this means the tests only test correctness, not performance.
  On many architectures, performance-related test results can be harder to reproduce, since hardware manufacturers
  may make architectural design decisions that improve average performance while introducing nondeterministic behaviour.
  The test results could depend on the branch predictor and/or cache state, rather than just the program code.

The tests work by tracking the current state and marking the line of code where one state ends and another begins.
Each state performs one or more checks, including a check to see if the next state is the expected one.
These state checks are coded in-line and rely on the compiler's string interning to check for equality.
The state checks are directly coded as string literals so other processes can check the state without using a global
  declaration at the top of the file, visible from both processes' functions.

Most tests run in the lowest-numbered process(es) possible.
For example, the only 2 tests in process 3 are the round-robin scheduling test and the resource contention tests.
FIFO-semantics for round robin scheduling are only noticeable when there are more than 2 processes, and the resource
  contention tests need at least 2 processes blocked on resources and 1 running process.
