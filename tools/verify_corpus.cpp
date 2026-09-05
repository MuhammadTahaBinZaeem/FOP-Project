#include "pocket_engineer/engine.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {
struct TopicSummary {
    std::uint64_t total{};
    std::uint64_t answer_correct{};
    std::uint64_t verification_correct{};
    std::uint64_t overall_correct{};
    std::uint64_t solver_errors{};
};

struct Report {
    std::uint64_t checked{};
    std::uint64_t answer_correct{};
    std::uint64_t verification_correct{};
    std::uint64_t overall_correct{};
    std::uint64_t solver_errors{};
    std::map<std::string,TopicSummary> topics;
    std::map<std::string,std::uint64_t> expected_verifications;
    std::vector<std::string> mismatch_examples;
};

std::optional<std::string> field(std::string_view json,std::string_view name) {
    const auto key="\""+std::string(name)+"\"";
    auto pos=json.find(key);
    if(pos==std::string_view::npos) return std::nullopt;
    pos=json.find(':',pos+key.size());
    if(pos==std::string_view::npos) return std::nullopt;
    ++pos;
    while(pos<json.size()&&(json[pos]==' '||json[pos]=='\t')) ++pos;
    if(pos>=json.size()||json[pos]!='\"') return std::nullopt;
    ++pos;
    std::string value;
    for(;pos<json.size();++pos) {
        const char c=json[pos];
        if(c=='"') return value;
        if(c!='\\') {
            value+=c;
            continue;
        }
        if(++pos>=json.size()) return std::nullopt;
        switch(json[pos]) {
            case 'n': value+='\n';break;
            case 'r': value+='\r';break;
            case 't': value+='\t';break;
            default: value+=json[pos];break;
        }
    }
    return std::nullopt;
}

void json_key_value(std::ostream& output,std::string_view key,std::uint64_t value,bool trailing=true) {
    output<<"    \""<<key<<"\": "<<value<<(trailing?",\n":"\n");
}

void write_report(const std::filesystem::path& path,const Report& report) {
    if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if(!output) throw std::runtime_error("Could not write report: "+path.string());
    output<<"{\n  \"schema_version\": \"1.0\",\n  \"kind\": \"golden_corpus_comparison\",\n";
    output<<"  \"summary\": {\n";
    json_key_value(output,"checked",report.checked);
    json_key_value(output,"answers_correct",report.answer_correct);
    json_key_value(output,"answers_incorrect",report.checked-report.answer_correct);
    json_key_value(output,"verification_correct",report.verification_correct);
    json_key_value(output,"verification_incorrect",report.checked-report.verification_correct);
    json_key_value(output,"overall_correct",report.overall_correct);
    json_key_value(output,"overall_incorrect",report.checked-report.overall_correct);
    json_key_value(output,"solver_errors",report.solver_errors,false);
    output<<"  },\n  \"expected_verification_statuses\": {";
    std::size_t count{};
    for(const auto& [name,value]:report.expected_verifications) {
        output<<(count++?",":"")<<"\n    \""<<pocket_engineer::json_escape(name)<<"\": "<<value;
    }
    if(!report.expected_verifications.empty()) output<<'\n';
    output<<"  },\n  \"topics\": [";
    count=0;
    for(const auto& [name,summary]:report.topics) {
        output<<(count++?",":"")<<"\n    {\"topic\": \""<<pocket_engineer::json_escape(name)<<"\", \"checked\": "<<summary.total<<", \"answers_correct\": "<<summary.answer_correct<<", \"verification_correct\": "<<summary.verification_correct<<", \"overall_correct\": "<<summary.overall_correct<<", \"solver_errors\": "<<summary.solver_errors<<"}";
    }
    if(!report.topics.empty()) output<<'\n';
    output<<"  ],\n  \"mismatch_examples\": [";
    for(std::size_t i=0;i<report.mismatch_examples.size();++i) output<<(i?",":"")<<"\n    \""<<pocket_engineer::json_escape(report.mismatch_examples[i])<<"\"";
    if(!report.mismatch_examples.empty()) output<<'\n';
    output<<"  ]\n}\n";
}
}

int main(int argc,char** argv) {
    const std::filesystem::path corpus=argc>1?argv[1]:"test-data";
    const std::uint64_t maximum=argc>2?std::stoull(argv[2]):0;
    const std::filesystem::path report_path=argc>3?argv[3]:"";
    if(!std::filesystem::is_directory(corpus)) {
        std::cerr<<"Corpus directory not found: "<<corpus<<"\n";
        return 2;
    }
    pocket_engineer::Engine engine;
    Report report;
    bool malformed=false;
    for(const auto& entry:std::filesystem::recursive_directory_iterator(corpus)) {
        if(!entry.is_regular_file()||entry.path().extension()!=".jsonl") continue;
        std::ifstream input(entry.path());
        std::string line;
        std::uint64_t line_number{};
        while(std::getline(input,line)) {
            ++line_number;
            const auto domain=field(line,"domain");
            const auto topic=field(line,"topic");
            const auto problem=field(line,"input");
            const auto answer=field(line,"expected_answer");
            const auto verification=field(line,"expected_verification");
            if(!domain||!topic||!problem||!answer||!verification) {
                malformed=true;
                if(report.mismatch_examples.size()<10) report.mismatch_examples.push_back(entry.path().string()+":"+std::to_string(line_number)+" malformed corpus row");
                continue;
            }
            const auto key=*domain+"/"+*topic;
            auto& topic_summary=report.topics[key];
            const auto result=engine.solve({*domain,*topic,*problem,{}});
            const bool answer_matches=result.status=="success"&&result.answer==*answer;
            const bool verification_matches=result.status=="success"&&pocket_engineer::verification_name(result.verification.status)==*verification;
            ++report.checked;
            ++topic_summary.total;
            ++report.expected_verifications[*verification];
            if(answer_matches) {
                ++report.answer_correct;
                ++topic_summary.answer_correct;
            }
            if(verification_matches) {
                ++report.verification_correct;
                ++topic_summary.verification_correct;
            }
            if(result.status!="success") {
                ++report.solver_errors;
                ++topic_summary.solver_errors;
            }
            if(answer_matches&&verification_matches) {
                ++report.overall_correct;
                ++topic_summary.overall_correct;
            } else if(report.mismatch_examples.size()<10) {
                report.mismatch_examples.push_back(entry.path().string()+":"+std::to_string(line_number)+" expected answer='"+*answer+"' actual='"+result.answer+"'");
            }
            if(maximum&&report.checked>=maximum) break;
        }
        if(maximum&&report.checked>=maximum) break;
    }
    if(!report_path.empty()) write_report(report_path,report);
    std::cout<<"Compared "<<report.checked<<" golden corpus cases: "<<report.overall_correct<<" matching, "<<report.checked-report.overall_correct<<" mismatching; answers "<<report.answer_correct<<"/"<<report.checked<<", verifications "<<report.verification_correct<<"/"<<report.checked<<".\n";
    return (!malformed&&report.checked>0&&report.overall_correct==report.checked)?0:(malformed?2:1);
}
