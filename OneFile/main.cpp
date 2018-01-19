#include <experimental/filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <locale>
#include <set>
#include <map>

using namespace std;
using namespace std::experimental::filesystem::v1;

string RemoveWhitespace(string &a) {
	string result;
	for (auto&& c : a) {
		if (!isspace(c)) result += c;
	}
	return result;
}

bool BeginsWith(string a, string b) {
	a = RemoveWhitespace(a);
	b = RemoveWhitespace(b);
	if (b.size() > a.size()) return false;
	for (int i = 0; i < b.size(); i++) {
		if (a[i] != b[i]) return false;
	}
	return true;
}

bool IsStandardInclude(string line) {
	return BeginsWith(line, "#include<");
}

string StringBetween(string s, char a, char b) {
	string result;
	bool between = false;
	for (auto&& c : s) {
		if (between) {
			if (c == b) break;
			result += c;
		}
		if (c == a) between = true;
	}
	return result;
}

string GetStandardInclude(string line) {
	return RemoveWhitespace(StringBetween(line, '<', '>'));
}

class SourceFile {

	set<string> standardIncludes;

public:
	SourceFile() { }

	SourceFile(string path) {
		ifstream file(path);
		string line;
		while (file.good()) {
			getline(file, line);
			if (IsStandardInclude(line)) {
				standardIncludes.insert(GetStandardInclude(line));
			}
		}
		file.close();
	}

	set<string> const& StandardIncludes() {
		return standardIncludes;
	}
};

class Project {

	map<string, SourceFile> files;
	set<string> standardIncludes;

public:
	Project(string directory) {
		for (auto&& i : recursive_directory_iterator(directory)) {
			string ext = i.path().extension().string();
			if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
				string path = i.path().string();
				files[path] = SourceFile(path);
				standardIncludes.insert(files[path].StandardIncludes().begin(), files[path].StandardIncludes().end());
			}
		}
	}

	void WriteStandardIncludes(ostream &os) {
		for (auto&& i : standardIncludes) {
			os << "#include <" << i << ">" << endl;
		}
	}
};

int main() {

	string directory = "C:/Users/Dominik/Programowanie/SDiZO/Project1";
	Project project(directory);
	project.WriteStandardIncludes(cout);

	system("PAUSE");
	return 0;
}