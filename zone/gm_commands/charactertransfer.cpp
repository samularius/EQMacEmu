void command_charactertransfer(Client* c, const Seperator* sep) {

	if (!c)
		return;

	const char* forum_name = c->ForumName();

	if (!forum_name || forum_name[0] == '\0')
	{
		c->Message(Chat::Red, "There is a problem with your character when issuing this command. Please contact a CSR.");
		return;
	}

	if (sep->arg[1][0] == 0)
		c->Message(Chat::White, "Usage: #charactertransfer [account_name]");
    else
    {
        if (sep->arg[1][0] == 0)
            c->Message(Chat::White, "Usage: #charactertransfer [account_name]");
        else
        {
            std::string user_target_account_name = "";
            for (int i = 0; i < sep->argnum; i++) {
                user_target_account_name += sep->arg[i];
                if (i + 1 < sep->argnum)
                    user_target_account_name += " ";
            }

            std::string target_forum_name = database.GetForumNameByAccountName(user_target_account_name.c_str(), false);

            if (target_forum_name.compare(forum_name) != 0)
            {
                c->Message(Chat::White, "You cannot transfer characters to an account your forum account does not own.");
                return;
            }

            uint32 target_account_id = database.GetAccountIDByName(user_target_account_name.c_str());
            char target_account_name[30];

            database.GetAccountFromID(target_account_id, target_account_name, nullptr, nullptr);

            if (target_account_id == 0 || target_account_name[0] == '\0')
            {
                c->Message(Chat::White, "Target account does not exist.");
                return;
            }

            if (target_account_id == c->AccountID())
            {
                c->Message(Chat::Red, "You cannot transfer a character to the account it is already on.");
                return;
            }

            int numChars = database.GetNumCharacters(target_account_id);

            if (numChars >= 8)
            {
                c->Message(Chat::White, "Account has too many characters.");
                return;
            }

            c->SetAccountName(target_account_name);
            c->SetAccountID(target_account_id);
            c->Save(1);
            c->Kick();
        }
    }
}