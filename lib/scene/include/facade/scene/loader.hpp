#include <djson/json.hpp>
#include <facade/scene/load_status.hpp>
#include <facade/scene/scene.hpp>
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

class Scene::Loader {
  public:
	Loader(Scene& out_scene, AtomicLoadStatus& out_status) : m_scene(out_scene), m_status(out_status) { m_status.reset(); }

	///
	/// \brief Load data from a GLTF file.
	/// \param root The root JSON node for the GLTF asset
	/// \param provider Data provider with the JSON parent directory mounted
	/// \param out_status Optional pointer to AtomicLoadStatus to be updated by the Scene
	/// \returns true If successfully loaded
	///
	/// If the GLTF data fails to load, the scene data will remain unchanged.
	/// This function purposely throws on fatal errors.
	///
	bool operator()(dj::Json const& json, DataProvider const& provider) noexcept(false);
	///
	/// \brief Obtain the load status.
	///
	AtomicLoadStatus const& load_status() const { return m_status; }

  private:
	Scene& m_scene;
	AtomicLoadStatus& m_status;
};
} // namespace facade
