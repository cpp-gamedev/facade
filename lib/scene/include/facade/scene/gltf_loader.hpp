#include <djson/json.hpp>
#include <facade/scene/load_status.hpp>
#include <facade/scene/scene.hpp>
#include <facade/util/thread_pool.hpp>
#include <atomic>

namespace facade {
///
/// \brief Concurrent status for multi-threaded loading
///
struct AtomicLoadStatus {
	std::atomic<LoadStage> stage{};
	std::atomic<std::size_t> total{};
	std::atomic<std::size_t> done{};

	void reset() {
		stage = LoadStage::eNone;
		total = {};
		done = {};
	}
};

template <typename T>
struct LoadFuture : MaybeFuture<T> {
	LoadFuture() = default;

	template <typename F>
	LoadFuture(ThreadPool& pool, std::atomic<std::size_t>& post_increment, F func)
		: MaybeFuture<T>(pool, [&post_increment, f = std::move(func)] {
			  auto ret = f();
			  ++post_increment;
			  return ret;
		  }) {}

	template <typename F>
	LoadFuture(std::atomic<std::size_t>& post_increment, F func) : MaybeFuture<T>(std::move(func)) {
		++post_increment;
	}
};

class Scene::GltfLoader {
  public:
	///
	/// \brief Construct a GltfLoader.
	/// \param out_scene The scene to load into
	/// \param out_status AtomicLoadStatus to be updated as the scene is loaded
	///
	GltfLoader(Scene& out_scene, AtomicLoadStatus& out_status) : m_scene(out_scene), m_status(out_status) { m_status.reset(); }

	///
	/// \brief Load data from a GLTF file.
	/// \param json The root JSON node for the GLTF asset
	/// \param provider Data provider with the JSON parent directory mounted
	/// \param thread_pool Optional pointer to thread pool to use for async loading of images and buffers
	/// \returns true If successfully loaded
	///
	/// If the GLTF data fails to load, the scene data will remain unchanged.
	/// This function purposely throws on fatal errors.
	///
	bool operator()(dj::Json const& json, DataProvider const& provider, ThreadPool* thread_pool = {}) noexcept(false);
	///
	/// \brief Obtain the load status.
	///
	AtomicLoadStatus const& load_status() const { return m_status; }

  private:
	Scene& m_scene;
	AtomicLoadStatus& m_status;
};
} // namespace facade
