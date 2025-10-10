#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include "discord.h"
#include "../http/httplib.h"
#include "../json/json.h"
#include "../strings.h"
#include "../eqemu_logsys.h"
#include "../events/player_event_logs.h"

constexpr int MAX_RETRIES = 10;

// AllowedMentions structure
struct AllowedMentions {
    std::vector<std::string> parse;
    std::vector<std::string> users;
    std::vector<std::string> roles;
    bool replied_user = false;

    // A serialize function is required by Cereal
    template <class Archive>
    void serialize(Archive& archive) {
        archive(
            CEREAL_NVP(parse),
            CEREAL_NVP(users),
            CEREAL_NVP(roles),
            CEREAL_NVP(replied_user)
        );
    }
};

// Message structure
struct DiscordMessage {
    std::string content;
    AllowedMentions allowed_mentions;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(
            CEREAL_NVP(content),
            CEREAL_NVP(allowed_mentions)
        );
    }
};

void Discord::SendWebhookMessage(const std::string& message, const std::string& webhook_url)
{
    // validate
    if (!ValidateWebhookUrl(webhook_url)) {
        return;
    }

    // split
    auto s = Strings::Split(webhook_url, '/');

    // url
    std::string base_url = fmt::format("{}//{}", s[0], s[2]);
    std::string endpoint = Strings::Replace(webhook_url, base_url, "");

    // client
    try {
        httplib::Client cli(base_url);
        cli.set_connection_timeout(0, 15000000); // 15 sec
        cli.set_read_timeout(15, 0); // 15 seconds
        cli.set_write_timeout(15, 0); // 15 seconds

        // payload
        DiscordMessage p;
        AllowedMentions am;
        am.parse.clear();
        am.users.clear();
        am.roles.clear();
        am.replied_user = false;
        p.content = message;
        p.allowed_mentions = am;

        // Create an output stream and a JSON archive
        std::ostringstream os;
        cereal::JSONOutputArchive archive(os);

        // Serialize the message object
        archive(p);
        std::stringstream payload;
        payload << os.str();

        bool retry = true;
        int  retries = 0;
        int  retry_timer = 1000;
        while (retry) {
            if (auto res = cli.Post(endpoint, payload.str(), "application/json")) {
                if (res->status != 200 && res->status != 204) {
                    LogError("[Discord Client] Code [{}] Error [{}]", res->status, res->body);
                }
                if (res->status == 429) {
                    if (!res->body.empty()) {
                        std::stringstream ss(res->body);
                        Json::Value       response;

                        try {
                            ss >> response;
                        }
                        catch (std::exception const& ex) {
                            LogDiscord("JSON serialization failure [{}] via [{}]", ex.what(), res->body);
                        }

                        retry_timer = Strings::ToInt(response["retry_after"].asString()) + 500;
                    }

                    LogDiscord("Rate limited... retrying message in [{}ms]", retry_timer);
                    std::this_thread::sleep_for(std::chrono::milliseconds(retry_timer + 500));
                }
                if (res->status == 204) {
                    retry = false;
                }
                if (retries > MAX_RETRIES) {
                    LogDiscord("Retries exceeded for message [{}]", message);
                    retry = false;
                }

                retries++;
            }
        }
    }
    catch (std::exception const& ex) {
        LogInfo("Client creation failure [{}]", ex.what());
        return;
    }
}

void Discord::SendPlayerEventMessage(
	const PlayerEvent::PlayerEventContainer &e,
	const std::string &webhook_url
)
{
	if (!ValidateWebhookUrl(webhook_url)) {
		return;
	}

	auto s = Strings::Split(webhook_url, '/');

	// url
	std::string base_url = fmt::format("{}//{}", s[0], s[2]);
	std::string endpoint = Strings::Replace(webhook_url, base_url, "");

	// client
	httplib::Client cli(base_url);
	cli.set_connection_timeout(0, 15000000); // 15 sec
	cli.set_read_timeout(15, 0); // 15 seconds
	cli.set_write_timeout(15, 0); // 15 seconds
	httplib::Headers headers = {
		{"Content-Type", "application/json"}
	};

	std::string payload = PlayerEventLogs::GetDiscordPayloadFromEvent(e);
	if (payload.empty()) {
		return;
	}

	bool retry = true;
	int  retries = 0;
	int  retry_timer = 1000;
	while (retry) {
		if (auto res = cli.Post(endpoint, payload, "application/json")) {
			if (res->status != 200 && res->status != 204) {
				LogError("Code [{}] Error [{}]", res->status, res->body);
			}
			if (res->status == 429) {
				if (!res->body.empty()) {
					std::stringstream ss(res->body);
					Json::Value       response;

					try {
						ss >> response;
					}
					catch (std::exception const &ex) {
						LogDiscord("JSON serialization failure [{}] via [{}]", ex.what(), res->body);
					}

					retry_timer = std::stoi(response["retry_after"].asString()) + 500;
				}

				LogDiscord("Rate limited... retrying message in [{}ms]", retry_timer);
				std::this_thread::sleep_for(std::chrono::milliseconds(retry_timer + 500));
			}
			if (res->status == 204) {
				retry = false;
			}
			if (retries > MAX_RETRIES) {
				LogDiscord("Retries exceeded for player event message");
				retry = false;
			}

			retries++;
		}
	}
}

std::string Discord::FormatDiscordMessage(uint16 category_id, const std::string& message)
{
	if (category_id == Logs::LogCategory::MySQLQuery) {
		return fmt::format("```sql\n{}\n```", message);
	}

	return message + "\n";
}

bool Discord::ValidateWebhookUrl(const std::string& webhook_url)
{
	// validate
	if (webhook_url.empty()) {
		LogDiscord("[webhook_url] is empty");
		return false;
	}

	// validate
	if (!Strings::Contains(webhook_url, "http://") && !Strings::Contains(webhook_url, "https://")) {
		LogDiscord("[webhook_url] [{}] does not contain a valid http/s prefix.", webhook_url);
		return false;
	}

	return true;
}
