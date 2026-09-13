# Test suite

GoogleTest/CTest suite over the **committed** stubs in `api/`. It needs no Docker, no proto
compiler and neither submodule — it builds and exercises the code that is in this repository.

```bash
make build_library   # compile + install the client package into the repo root
make unit_test       # build and run the suite
make coverage        # same, under gcov, failing below COVERAGE_MIN % line coverage
make test            # check_stubs -> check_build -> unit_test -> smoke_test
```

## Layout

| File | Product-specific? | What it is |
| --- | --- | --- |
| `product_config.{h,cc}` | **yes** | The expectation tables: proto files, services, a slice of the RPC surface, the sweep floors |
| `descriptor_probe.{h,cc}` | no | Reflection helpers over the generated descriptor pool |
| `test_generated_stubs.cc` | mostly no | Pool-wide assertions driven entirely by `product_config.cc`, plus one extra case (below) |
| `test_typed_api.cc` | **yes** | Assertions against concrete `ondewo::survey` C++ types |
| `CMakeLists.txt` | no | Standalone project consuming the installed CMake package |

## What it actually checks

SURVEY is a small, self-contained product: **4** `.proto` files and **2** services. Those counts
are two orders of magnitude below the NLU client's and that is CORRECT — it is not a sign that
generation dropped anything.

- Every `.proto` listed in `product_config.cc` is registered, both services exist and declare
  RPCs, and a representative slice of method names is present — the full `Survey` CRUD surface,
  the answer-retrieval and agent-lifecycle RPCs, and all three RPCs of the second service,
  `ondewo.survey.FHIR`, which lives in its own `.proto`.
- **Every** generated message is instantiated, has each of its singular scalar fields set to a
  non-default value, and is pushed through `SerializeToString` → `ParseFromString` → compare:
  **29** messages and **60** singular scalar fields at ONDEWO SURVEY API 2.0.0. A writer that
  drops a field, a reader that ignores one, or a field-number mismatch between the two fails here.
- Both generated enums — the top-level `SubFlow` and the nested `Survey.AgentStatus` — declare
  `0` as their first value, as proto3 requires. A floor of 2 in `product_config.cc` is the honest
  one here; raising it would only make the gate unsatisfiable.
- `FillScalarFields` handles every protobuf scalar type. No single product uses all of them, so
  the branches are pinned against `google.protobuf`'s wrapper types, which libprotobuf registers
  into the same pool. That is what lets `descriptor_probe.cc` be copied between products untouched.
- Service stubs are constructed against a channel, and a unary RPC on **each** of the two
  services (`Surveys.GetSurvey`, `FHIR.GetAllFHIRSurveyAnswers`) is actually issued against a
  dead endpoint: each must come back as `UNAVAILABLE` / `DEADLINE_EXCEEDED`, which proves the
  stubs, the request/response types and the generated method descriptors all link and dispatch.

### Two shapes that do not exist in this product

Both are **left out** rather than faked, and their absence is a property of the API, not a gap:

- **proto3 explicit presence.** The SURVEY API declares no `optional` field at all
  (`grep -rn '^[[:space:]]*optional ' ondewo-survey-api/ondewo` is empty), so there is no
  presence bit in these stubs to assert on. `TypedApi.OneofArmSurvivesItsZeroValue` covers the
  nearest real thing: a `oneof` arm set to its type default must still come back as *set*.
- **Streaming.** Both services are unary end to end (`grep -n 'rpc .*stream'` is empty), so no
  `ClientReader` / `ClientWriter` / `ClientReaderWriter` is generated to drive.

### The one extra pool-wide case

`test_generated_stubs.cc` carries one test that is **not** in the `ondewo-nlu-client-cpp`
original: `MessagesInFileSkipsSyntheticMapEntryTypes`. `descriptor_probe.cc` filters out the
synthetic map-entry type protoc emits for every `map<>` field — it has no standalone generated
C++ class, so the sweep could not instantiate it — but the SURVEY protos declare no `map<>` field
at all, which would leave that filter unexercised here and drop line coverage below the gate.
It is therefore pinned against `google/protobuf/struct.proto`, which libprotobuf registers into
the same pool (`ondewo/survey/fhir.proto` imports it) and whose `google.protobuf.Struct` declares
exactly one map field. Same trick as the wrapper-type test above, applied to the other
product-dependent branch — fixed at the root rather than by lowering the threshold.

## Notes on the build

- The suite is a **standalone** CMake project and is not `add_subdirectory()`-ed from the root
  `CMakeLists.txt`: that file is shipped verbatim by the compiler image and is overwritten on
  every regeneration. Consuming the installed package instead is also the stronger test.
- It links the client archive with `--whole-archive` (`-force_load` on macOS). A static archive
  otherwise contributes only the objects needed to resolve a referenced symbol, and the
  descriptor registration of a generated `*.pb.o` is a static initialiser nothing references —
  without it the pool-wide sweeps would silently check only the handful of files
  `test_typed_api.cc` names.
- It compiles with `-fno-exceptions`. Nothing here throws, and the compiler-generated unwind
  blocks otherwise land on every closing brace as lines no test can reach.

## Coverage

`make coverage` measures **hand-written** code only: the gcovr filter is `tests/`, and the
client archive is compiled without `--coverage`, so no generated `*.pb.cc` can contribute a
line either way. The floor is 100 % lines (`COVERAGE_MIN`) and CI enforces it — measured
**100 % of 304 lines and 70 of 70 functions**. The generated stubs are excluded from the
*metric* but are the *subject* of every test above.
