
set(npm_setup_dependencies)

setup_mocha_test_dir(
  OUT_DEPENDS_VAR npm_setup_dependencies
  DIRECTORY "client"
)

add_mocha_test(
  NAME System::Diagsession
  DIRECTORY "client"
  SPECS
    tests/diagsession.test.ts
)

add_mocha_test(
  NAME System::PerfData
  DIRECTORY "client"
  SPECS
    tests/perf_data.test.ts
)

add_custom_target(npm_install ALL
  DEPENDS
    ${npm_setup_dependencies}
)
