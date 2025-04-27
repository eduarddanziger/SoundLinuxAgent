#pragma once

std::string SodiumDecrypt(const std::string& ciphertext, const std::string& key);

std::string SodiumEncrypt(const std::string& plaintext, const std::string& key);
