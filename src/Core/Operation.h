#pragma once

#include <iiGraphicAdjust/LocalCorrections.h>
#include <concepts>
#include <memory>
#include <optional>
#include <utility>

namespace iiGraphicAdjust {

struct OperationInfo final {
    std::string_view id, name, family; // Built-in metadata has static lifetime.
};

template<class T>
concept ImageOperationObject = requires(const T &object, ImageView image) {
    { T::info } -> std::convertible_to<OperationInfo>;
    { object.apply(image) } -> std::same_as<Result<Image>>;
} && std::copy_constructible<T>;

// An immutable snapshot of one concrete value object, including its owned resources.
// Copying the handle shares that const snapshot; subsequent edits to the original
// concrete object do not change it. Construction follows normal C++ allocation rules.
class Operation final {
    struct Callable {
        virtual ~Callable() = default;
        virtual Result<Image> apply(ImageView) const = 0;
        virtual OperationInfo info() const noexcept = 0;
    };
    template<class T> struct Model final : Callable {
        const T object;
        explicit Model(T value) : object(std::move(value)) {}
        Result<Image> apply(ImageView image) const override { return object.apply(image); }
        OperationInfo info() const noexcept override { return T::info; }
    };
    std::shared_ptr<const Callable> callable_;
public:
    template<ImageOperationObject T>
    explicit Operation(T object) : callable_(std::make_shared<Model<T>>(std::move(object))) {}
    Operation(const Operation &) noexcept = default;
    Operation(Operation &&) noexcept = default;
    Operation &operator=(const Operation &) noexcept = default;
    Operation &operator=(Operation &&) noexcept = default;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> apply(ImageView) const;
    [[nodiscard]] OperationInfo info() const noexcept {
        return callable_ ? callable_->info() : OperationInfo{"operation.empty", "Empty operation", "utility"};
    }
};

struct OperationStage final {
    Operation operation;
    bool enabled = true;
    double amount = 1; // [0,1]; image interpolation, not source-over opacity.
    std::optional<Mask> mask; // Owned, in the input coordinates of this stage.
    CorrectionSpace space = CorrectionSpace::LinearSrgb;
};

struct OperationFailure final {
    // max(size_t) denotes stack/input validation before a stage has been selected.
    std::size_t stage = static_cast<std::size_t>(-1);
    OperationInfo operation{};
    Error cause;
};

struct OperationStack final {
    static constexpr std::size_t maxStages = 256;
    std::vector<OperationStage> stages;
    // Whole-image stages may change geometry. Masked/partial stages must retain it.
    // Interpolation uses premultiplied RGB/alpha in the selected transfer space.
    [[nodiscard]] IIGRAPHICADJUST_EXPORT std::expected<Image, OperationFailure>
    evaluate(ImageView) const;
    [[nodiscard]] IIGRAPHICADJUST_EXPORT Result<Image> apply(ImageView) const;
};

// Concrete processing, mask and analysis objects. This is an inventory, not a
// string-dispatch factory; callers construct the appropriate typed object.
[[nodiscard]] IIGRAPHICADJUST_EXPORT std::span<const OperationInfo> operationCatalog() noexcept;

} // namespace iiGraphicAdjust
