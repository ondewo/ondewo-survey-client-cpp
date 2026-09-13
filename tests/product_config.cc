#include "product_config.h"

namespace ondewo_client_test {

// Mirrors the #include list of the generated public-api.h, one .proto per pair of headers:
//   sed -n 's|^#include "\(.*\)\.pb\.h"$|\1|p' public-api.h | sed 's|\.grpc$||' | sort -u
//
// Survey is a small, self-contained product: two .proto files of its own plus the two
// google.api annotation files their HTTP option bindings import. A short list is CORRECT here
// - it is not a sign that generation dropped anything.
const std::vector<std::string> kProtoFileNames = {
    "google/api/annotations.proto",
    "google/api/http.proto",
    "ondewo/survey/fhir.proto",
    "ondewo/survey/survey.proto",
};

const std::vector<std::string> kServiceFullNames = {
    "ondewo.survey.FHIR",
    "ondewo.survey.Surveys",
};

const std::vector<ExpectedMethod> kExpectedMethods = {
    // The complete CRUD surface of the product's main service ...
    {"ondewo.survey.Surveys", "CreateSurvey"},
    {"ondewo.survey.Surveys", "GetSurvey"},
    {"ondewo.survey.Surveys", "UpdateSurvey"},
    {"ondewo.survey.Surveys", "DeleteSurvey"},
    {"ondewo.survey.Surveys", "ListSurveys"},
    // ... the RPCs the product exists for, reading back what respondents answered ...
    {"ondewo.survey.Surveys", "GetSurveyAnswers"},
    {"ondewo.survey.Surveys", "GetAllSurveyAnswers"},
    // ... the NLU-agent lifecycle a survey drives ...
    {"ondewo.survey.Surveys", "CreateAgentSurvey"},
    {"ondewo.survey.Surveys", "UpdateAgentSurvey"},
    {"ondewo.survey.Surveys", "DeleteAgentSurvey"},
    // ... and the second service in the same library, in its own .proto.
    {"ondewo.survey.FHIR", "CreateFHIRSurvey"},
    {"ondewo.survey.FHIR", "GetFHIRSurveyAnswers"},
    {"ondewo.survey.FHIR", "GetAllFHIRSurveyAnswers"},
};

const std::string kScalarMessageFullName = "ondewo.survey.Survey";

const std::string kEnumFullName = "ondewo.survey.SubFlow";

// ONDEWO SURVEY API 2.0.0 generates 29 messages (map entries excluded), 2 enums and 60
// singular scalar fields across the files listed above. The floors sit just below that.
//
// These are two orders of magnitude smaller than the NLU client's and that is correct: survey
// declares exactly two .proto files and exactly two enums (the top-level SubFlow and the
// nested Survey.AgentStatus), so a floor of 2 is the honest one - raising it would only make
// the gate unsatisfiable.
const int kMinimumMessageCount = 27;
const int kMinimumEnumCount = 2;
const int kMinimumScalarFieldCount = 55;

}  // namespace ondewo_client_test
