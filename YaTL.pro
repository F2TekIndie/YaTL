TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = core app cli core_tests ui_tests
core.file = src/core/core.pro
app.file = app/app.pro
cli.file = src/cli/cli.pro
core_tests.file = tests/core/core_tests.pro
ui_tests.file = tests/ui/ui_tests.pro
