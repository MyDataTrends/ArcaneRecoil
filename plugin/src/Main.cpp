#include "ArcaneRecoil.h"
#include "PCH.h"

// =============================================================================
// SKSE Plugin Entry Point
// =============================================================================

// Tells SKSE which game version(s) this plugin supports.
// CommonLibSSE-NG's SKSEPluginLoad macro handles the boilerplate.
SKSEPluginLoad(const SKSE::LoadInterface *skse) {
  // Initialise SKSE and set up logging.
  SKSE::Init(skse);

  // Set up spdlog - writes to Documents/My Games/Skyrim Special Edition/SKSE/
  const auto pluginPath = SKSE::log::log_directory();
  if (pluginPath) {
    auto path = *pluginPath / "ArcaneRecoil.log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        path.string(), true);
    auto logger =
        std::make_shared<spdlog::logger>("ArcaneRecoil", std::move(sink));
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::warn);  // v1.1: no forced flush per info line
    spdlog::set_default_logger(std::move(logger));
    spdlog::flush_every(std::chrono::seconds(2));  // periodic, so a crash or quit keeps recent lines
  }

  LOG_INFO("Arcane Recoil v{}.{}.{} loading...",
           SKSE::PluginDeclaration::GetSingleton()->GetVersion().major(),
           SKSE::PluginDeclaration::GetSingleton()->GetVersion().minor(),
           SKSE::PluginDeclaration::GetSingleton()->GetVersion().patch());

  // Register a listener for SKSE messages.
  // kDataLoaded fires after all .esp/.esm/.esl files have been loaded into
  // memory — the first safe moment to look up FormIDs.
  SKSE::GetMessagingInterface()->RegisterListener(
      [](SKSE::MessagingInterface::Message *msg) {
        if (msg->type == SKSE::MessagingInterface::kDataLoaded) {
          ArcaneRecoil::Install();
        } else if (msg->type == SKSE::MessagingInterface::kPostLoadGame ||
                   msg->type == SKSE::MessagingInterface::kNewGame) {
          ArcaneRecoil::OnGameLoaded();  // the player's animation graph is new: re-attach the cast-start listener
        }
      });

  return true;
}
