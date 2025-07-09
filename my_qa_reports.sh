#!/bin/bash

# The CDash pipeline: generate the coverage report and run testing.
# We can use that in our favor prior to running SonarScanner.
# It also runs memcheck and creates the valgrind report
echo "===== Starting CDash pipeline with valgrind ====="

ctest -VV -S Pipeline.cmake > CDashSummary.log

echo "===== CDash report submitted successfully ====="

echo "===== Starting SonarScanner ====="

# Now we generate each report

# Generate the test report for sonar.cxx.xunit.reportPaths=PATH_TO/ctest_report.xml
ctest --test-dir build/tests --output-junit ctest_report.xml

if [ -f build/tests/ctest_report.xml ]; then
  echo "=== Test report generated ==="
else
  echo "=== Test report failed ==="
  exit 1
fi

# Generate the coverage report for sonar.cxx.coverage.reportPaths=PATH_TO/coverage.xml
gcovr -r . --xml-pretty --output=build/coverage.xml

if [ -f build/coverage.xml ]; then
  echo "=== Coverage report generated ==="
else
  echo "=== Coverage report failed ==="
  exit 1
fi

# Generate the cppcheck report for sonar.cxx.cppcheck.reportPaths=PATH_TO/cppcheck.xml
cppcheck --enable=all --xml --xml-version=2 src lib 2>build/cppcheck.xml

if [ -f build/cppcheck.xml ]; then
  echo "=== Cppcheck report generated ==="
else
  echo "=== Cppcheck report failed ==="
  exit 1
fi

# Only checks the valgrind report for sonar.cxx.valgrind.reportPaths=PATH_TO/valgrind.xml
echo "=== Checking MemCheck (Valgrind) Report via CTest ==="

if [ -f build/valgrind_report.xml ]; then
  echo "=== Valgrind report was generated succesfully==="
else
  echo "=== Valgrind report failed ==="
  exit 1
fi


 # Generate the scanbuild report for sonar.cxx.scanbuild.reportPaths=PATH_TO/scanbuild.xml
 scan-build --status-bugs -plist --force-analyze-debug-code -analyze-headers -enable-checker alpha.core.CastSize -enable-checker alpha.core.CastToStruct -enable-checker alpha.core.Conversion -enable-checker alpha.core.IdenticalExpr -enable-checker alpha.core.SizeofPtr -enable-checker alpha.core.TestAfterDivZero --exclude external -o build/analyzer_reports  ninja -C build 

if [ -d build/analyzer_reports ]; then
  echo "=== Scanbuild report generated ==="
else
  echo "=== Scanbuild report failed ==="
  exit 1
fi

# Generate Clang tidy report for sonar.cxx.clangtidy.reportPaths=PATH_TO/clangtidy.xml
python3 run-clang-tidy.py -checks='*' -p='build' -header-filter='*' src >build/clangtidy.txt

if [ -f build/clangtidy.txt ]; then
  echo "=== Clang tidy report generated ==="
else
  echo "=== Clang tidy report failed ==="
  exit 1
fi

echo "==== All reports generated correctly ===="
sonar-scanner -Dsonar.token=sqp_72ddb1568c1a1426da78121071453db0fde03256
exit 0