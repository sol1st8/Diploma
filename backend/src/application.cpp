#include "application.h"

namespace db {

Application::Application(const AppConfig& config)
    : db_(config.db_url) {}

app::UseCasesImpl Application::GetUseCases() const {
    return use_cases_;
}

} // namespace db
