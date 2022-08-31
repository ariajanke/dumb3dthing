#include <iostream>
#include <vector>
#include <array>

#include <emscripten.h>

static std::vector<std::byte> s_buffer;
constexpr const std::array<float, 5> k_secret_array = {
	4.f, 3.f, 2.f, 1.f, 5.f
};

EM_JS(void, from_js_pass_secret_array, (const float * fptr, int length), {
	Module.HEAPF32.slice((fptr / 4), (fptr / 4) + length).forEach((n, idx) => {
		console.log(`secret array ${idx}: ${n}`);
	});
});

int main() {
	using std::cout, std::endl;
	cout << "Hello World!" << endl;
}

extern "C" {

float to_js_sum_array(float * arr, int length) {
	auto * end = arr + length;
	float s = 0;
	for (auto * itr = arr; itr != end; ++itr) {
		s += *itr;
	}
	return s;
}

void * to_js_reserve_temporary_buffer(std::size_t length) {
	s_buffer.resize(length);
	return s_buffer.data();
}

void to_js_get_secret_array() {
	from_js_pass_secret_array(&k_secret_array.front(), k_secret_array.size());
}

}