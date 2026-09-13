// Assertions against the concrete C++ types the SURVEY stubs generate.
//
// This is the per-product half of the suite: it names ondewo::survey types, so replicating the
// suite to another ONDEWO client means rewriting this file against that product's messages and
// services. Everything generic lives in test_generated_stubs.cc.
//
// Two of the shapes the NLU client tests have no counterpart here, and they are LEFT OUT
// rather than faked:
//   * proto3 explicit presence - the SURVEY API declares no `optional` field at all
//     (`grep -rn '^[[:space:]]*optional ' ondewo-survey-api/ondewo` is empty), so there is no
//     presence bit in these stubs to assert on.
//   * streaming - both SURVEY services are unary end to end (`grep -n 'rpc .*stream'` is
//     empty), so no ClientReader / ClientWriter / ClientReaderWriter is generated.

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "ondewo/survey/fhir.grpc.pb.h"
#include "ondewo/survey/survey.grpc.pb.h"
#include "ondewo/survey/survey.pb.h"

namespace ondewo_client_test {
namespace {

// A channel to a port nothing listens on. gRPC connects lazily, so constructing stubs
// against it touches no network at all; the one test that does issue an RPC gives it a
// short deadline and asserts only that the call comes back as a failure.
std::shared_ptr<grpc::Channel> DeadChannel() {
  return grpc::CreateChannel("127.0.0.1:1", grpc::InsecureChannelCredentials());
}

TEST(TypedApi, MessageSurvivesSerializeAndParse) {
  ondewo::survey::Survey original;
  original.set_survey_id("projects/a-project/agent");
  original.set_display_name("Customer satisfaction");
  original.set_language_code("de");
  original.set_status(ondewo::survey::Survey::UPDATED);
  original.add_exclude_subflows(ondewo::survey::SubFlow::PHONE_HOURS);

  ondewo::survey::SurveyInfo* info = original.mutable_survey_info();
  info->set_legal_entity("ONDEWO GmbH");
  info->set_topic("satisfaction");
  info->set_anonymous(true);

  ondewo::survey::OpenQuestion* question =
      original.add_questions()->mutable_open_question();
  question->set_question_text("How did we do?");

  std::string bytes;
  ASSERT_TRUE(original.SerializeToString(&bytes));
  EXPECT_FALSE(bytes.empty());

  ondewo::survey::Survey parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));

  EXPECT_EQ(parsed.survey_id(), "projects/a-project/agent");
  EXPECT_EQ(parsed.display_name(), "Customer satisfaction");
  EXPECT_EQ(parsed.language_code(), "de");
  EXPECT_EQ(parsed.status(), ondewo::survey::Survey::UPDATED);
  ASSERT_EQ(parsed.exclude_subflows_size(), 1);
  EXPECT_EQ(parsed.exclude_subflows(0), ondewo::survey::SubFlow::PHONE_HOURS);
  EXPECT_EQ(parsed.survey_info().legal_entity(), "ONDEWO GmbH");
  EXPECT_EQ(parsed.survey_info().topic(), "satisfaction");
  EXPECT_TRUE(parsed.survey_info().anonymous());
  ASSERT_EQ(parsed.questions_size(), 1);
  EXPECT_EQ(parsed.questions(0).question_case(), ondewo::survey::Question::kOpenQuestion);
  EXPECT_EQ(parsed.questions(0).open_question().question_text(), "How did we do?");
  EXPECT_EQ(parsed.SerializeAsString(), bytes);
}

// A oneof alternative has presence even in a product that declares no `optional` field: which
// arm is set must survive the wire, and a bool arm set to false must still come back as SET
// rather than collapsing into "no arm chosen".
TEST(TypedApi, OneofArmSurvivesItsZeroValue) {
  ondewo::survey::Answer original;
  original.set_question_nr(1);
  original.set_answer_text("yes, please");
  original.set_anonymous(false);
  ASSERT_EQ(original.is_anonymous_case(), ondewo::survey::Answer::kAnonymous);

  const std::string bytes = original.SerializeAsString();
  EXPECT_FALSE(bytes.empty());

  ondewo::survey::Answer parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));
  EXPECT_EQ(parsed.is_anonymous_case(), ondewo::survey::Answer::kAnonymous)
      << "a oneof arm holding the type default was lost on the wire";
  EXPECT_FALSE(parsed.anonymous());

  original.clear_is_anonymous();
  EXPECT_EQ(original.is_anonymous_case(), ondewo::survey::Answer::IS_ANONYMOUS_NOT_SET);
}

// A plain (non-optional) proto3 scalar: its zero value is the field default and must NOT be
// written. This is the contract every field in these stubs follows, since the SURVEY API
// declares no explicit-presence field to contrast it with.
TEST(TypedApi, PlainScalarZeroValueStaysOffTheWire) {
  ondewo::survey::Answer answer;
  answer.set_question_nr(0);
  EXPECT_TRUE(answer.SerializeAsString().empty());

  answer.set_question_nr(7);
  EXPECT_FALSE(answer.SerializeAsString().empty());
}

TEST(TypedApi, EnumZeroValueIsTheUnspecifiedOne) {
  EXPECT_EQ(static_cast<int>(ondewo::survey::SubFlow::SUBFLOW_UNSPECIFIED), 0);
  EXPECT_EQ(ondewo::survey::SubFlow_Name(ondewo::survey::SubFlow::SUBFLOW_UNSPECIFIED),
            "SUBFLOW_UNSPECIFIED");
  EXPECT_EQ(static_cast<int>(ondewo::survey::Survey::TO_BE_INITIALIZED), 0);

  ondewo::survey::SubFlow parsed = ondewo::survey::SubFlow::BOT;
  ASSERT_TRUE(ondewo::survey::SubFlow_Parse("SUBFLOW_UNSPECIFIED", &parsed));
  EXPECT_EQ(parsed, ondewo::survey::SubFlow::SUBFLOW_UNSPECIFIED);

  // A fresh Survey defaults to the zero agent status, so the zero value has to be usable.
  ondewo::survey::Survey survey;
  EXPECT_EQ(survey.status(), ondewo::survey::Survey::TO_BE_INITIALIZED);
}

TEST(TypedApi, ServiceStubsAreConstructibleAgainstAChannel) {
  const std::shared_ptr<grpc::Channel> channel = DeadChannel();
  ASSERT_NE(channel, nullptr);

  std::unique_ptr<ondewo::survey::Surveys::Stub> surveys =
      ondewo::survey::Surveys::NewStub(channel);
  std::unique_ptr<ondewo::survey::FHIR::Stub> fhir = ondewo::survey::FHIR::NewStub(channel);

  EXPECT_NE(surveys, nullptr);
  EXPECT_NE(fhir, nullptr);
}

TEST(TypedApi, ServicesKeepTheirFullyQualifiedNames) {
  EXPECT_STREQ(ondewo::survey::Surveys::service_full_name(), "ondewo.survey.Surveys");
  EXPECT_STREQ(ondewo::survey::FHIR::service_full_name(), "ondewo.survey.FHIR");
}

// Actually issue an RPC. Nothing is listening, so the only correct outcome is a failure -
// but reaching a transport-level failure means the stub, the request/response types and
// the generated method descriptor all linked and dispatched. A crash or an OK here would
// mean the generated client is broken.
TEST(TypedApi, UnaryRpcAgainstADeadEndpointFailsCleanly) {
  std::unique_ptr<ondewo::survey::Surveys::Stub> surveys =
      ondewo::survey::Surveys::NewStub(DeadChannel());

  grpc::ClientContext client_context;
  client_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

  ondewo::survey::GetSurveyRequest request;
  request.set_survey_id("projects/a-project/agent");
  ondewo::survey::Survey response;

  const grpc::Status status = surveys->GetSurvey(&client_context, request, &response);

  EXPECT_FALSE(status.ok()) << "an RPC to a dead endpoint reported success";
  EXPECT_TRUE(status.error_code() == grpc::StatusCode::UNAVAILABLE ||
              status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
      << "unexpected status " << status.error_code() << ": " << status.error_message();
}

// The second service lives in its own .proto and its own generated stub - a unary RPC on it
// too, so a fhir.grpc.pb.cc that failed to link would not hide behind the Surveys stub.
TEST(TypedApi, SecondServiceUnaryRpcAgainstADeadEndpointFailsCleanly) {
  std::unique_ptr<ondewo::survey::FHIR::Stub> fhir =
      ondewo::survey::FHIR::NewStub(DeadChannel());

  grpc::ClientContext client_context;
  client_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

  ondewo::survey::GetAllSurveyAnswersRequest request;
  request.set_survey_id("projects/a-project/agent");
  ondewo::survey::SurveyFHIRAnswersResponse response;

  const grpc::Status status =
      fhir->GetAllFHIRSurveyAnswers(&client_context, request, &response);

  EXPECT_FALSE(status.ok()) << "an RPC to a dead endpoint reported success";
  EXPECT_TRUE(status.error_code() == grpc::StatusCode::UNAVAILABLE ||
              status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
      << "unexpected status " << status.error_code() << ": " << status.error_message();
}

}  // namespace
}  // namespace ondewo_client_test
