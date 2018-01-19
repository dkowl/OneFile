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

bool IsStandardInclude(string line) {
	return BeginsWith(line, "#include<");
}

bool IsCustomInclude(string line) {
	return BeginsWith(line, "#include\"");
}

string GetStandardInclude(string line) {
	return RemoveWhitespace(StringBetween(line, '<', '>'));
}

string GetCustomInclude(string line) {
	return RemoveWhitespace(StringBetween(line, '"', '"'));
}

class SourceFile {

	set<string> standardIncludes;
	set<string> headers;
	vector<string> content;

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
			else if (IsCustomInclude(line)) {
				headers.insert(GetCustomInclude(line));
			}
			else {
				content.push_back(line);
			}
		}
		file.close();
	}

	set<string> const& StandardIncludes() {
		return standardIncludes;
	}

	set<string> const& Headers() {
		return headers;
	}

	void WriteContent(ostream &os) {
		for (auto&& line : content) {
			os << line << endl;
		}
	}
};

template<typename TKey>
class DependencyGraph {

	class Node;

	map<TKey, Node> graph;

public:

	void AddEdge(TKey from, TKey to) {
		graph[from].requires.insert(to);
		graph[to].isRequiredBy.insert(from);
	}

	void RemoveNode(TKey key) {
		for (auto&& from : graph[key].isRequiredBy) {
			graph[from].requires.erase(key);
		}
		graph.erase(key);
	}

	vector<TKey> Solve() {
		vector<TKey> result;
		while (!graph.empty()) {
			for (auto&& i : graph) {
				if (i.second.IsRoot()) {
					result.push_back(i.first);
					RemoveNode(i.first);
					break;
				}
			}
		}
		return result;
	}

	class Node {
		friend class DependencyGraph;

		set<TKey> requires;
		set<TKey> isRequiredBy;

		bool IsRoot() {
			return requires.empty();
		}
	};
};

class Project {

	class GraphNode;

	map<string, SourceFile> files;
	set<string> standardIncludes;
	DependencyGraph<string> dependencyGraph;

public:
	Project(string directory) {
		for (auto&& i : recursive_directory_iterator(directory)) {

			string ext = i.path().extension().string();
			if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
				string curPath = i.path().string();
				files[curPath] = SourceFile(curPath);

				standardIncludes.insert(files[curPath].StandardIncludes().begin(), files[curPath].StandardIncludes().end());


				for (auto&& header : files[curPath].Headers()) {
					path headerPath = i.path();
					headerPath.replace_filename(header);
					dependencyGraph.AddEdge(curPath, headerPath.string());
				}
			}
		}
	}

	void WriteStandardIncludes(ostream &os) {
		for (auto&& i : standardIncludes) {
			os << "#include <" << i << ">" << endl;
		}
	}

	void WriteFile(ostream &os, string filepath) {
		files[filepath].WriteContent(os);
	}

	void Write(ostream &os) {
		WriteStandardIncludes(os);

		vector<string> sourcePaths = dependencyGraph.Solve();
		for (auto&& sourcePath : sourcePaths) {
			WriteFile(os, sourcePath);
		}
	}
};

int main() {

	string directory = current_path().string();
	Project project(directory);

	ofstream file("OneFile.cpp");
	project.Write(file);
	file.close();

	return 0;
}