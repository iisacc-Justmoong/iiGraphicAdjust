#include <iiGraphicAdjust/Operation.h>
#include "MaskInternal.h"

namespace iiGraphicAdjust {
Result<Image> Operation::apply(ImageView image) const {
    if (!callable_)
        return std::unexpected(Error{ErrorCode::InvalidParameter, "Operation was moved from"});
    try { return callable_->apply(image); }
    catch (const std::bad_alloc &) {
        return std::unexpected(Error{ErrorCode::AllocationFailure, "Could not allocate operation storage"});
    } catch (const std::length_error &) {
        return std::unexpected(Error{ErrorCode::AllocationFailure, "Operation storage exceeds container limits"});
    }
}

std::expected<Image, OperationFailure> OperationStack::evaluate(ImageView source) const {
    std::size_t index = static_cast<std::size_t>(-1);
    OperationInfo selected{};
    const auto fail = [&](Error error) -> std::expected<Image,OperationFailure> {
        return std::unexpected(OperationFailure{index,selected,error});
    };
    if (auto valid=detail::validate(source); !valid) return fail(valid.error());
    if (stages.size()>maxStages)
        return fail({ErrorCode::InvalidParameter,"Operation stack exceeds 256 stages"});
    try {
        Image current=detail::copy(source);
        for (index=0; index<stages.size(); ++index) {
            const auto &stage=stages[index]; selected=stage.operation.info();
            if (!detail::inRange(stage.amount,0,1) ||
                (stage.space!=CorrectionSpace::Srgb && stage.space!=CorrectionSpace::LinearSrgb))
                return fail({ErrorCode::InvalidParameter,"Invalid operation stage interpolation"});
            if (stage.mask) {
                if (auto valid=detail::validateMask(stage.mask->view()); !valid) return fail(valid.error());
                if (stage.mask->width!=current.width || stage.mask->height!=current.height)
                    return fail({ErrorCode::SizeMismatch,"Stage mask must match its input image"});
            }
            if (!stage.enabled) continue;
            auto corrected=stage.operation.apply(current.view());
            if (!corrected) return fail(corrected.error());
            if (auto valid=detail::validate(corrected->view()); !valid) return fail(valid.error());
            if (!stage.mask && stage.amount==1) {
                current=std::move(*corrected); continue;
            }
            if (corrected->width!=current.width || corrected->height!=current.height)
                return fail({ErrorCode::SizeMismatch,"Masked or partial operations cannot change geometry"});
            if (stage.amount==0) continue;
            for (std::size_t i=0;i<current.pixels.size();++i) {
                const auto weight=stage.amount*(stage.mask ? stage.mask->coverage[i] : 1.);
                if (weight==0) continue;
                auto &p=current.pixels[i]; const auto q=corrected->pixels[i];
                if (weight==1) { p=q; continue; }
                if (p==q) continue;
                const auto alpha=(1-weight)*p.a+weight*q.a;
                if (alpha==0) continue; // Retain the input's invisible RGB deterministically.
                const auto mix=[&](float a,float b) -> float {
                    if (stage.space==CorrectionSpace::LinearSrgb)
                        return detail::encoded(((1-weight)*detail::linear(a)*p.a+
                                                weight*detail::linear(b)*q.a)/alpha);
                    return static_cast<float>(detail::clamp(((1-weight)*a*p.a+weight*b*q.a)/alpha));
                };
                p={mix(p.r,q.r),mix(p.g,q.g),mix(p.b,q.b),static_cast<float>(alpha)};
            }
        }
        return current;
    } catch (const std::bad_alloc &) {
        return fail({ErrorCode::AllocationFailure,"Could not allocate stack storage"});
    } catch (const std::length_error &) {
        return fail({ErrorCode::AllocationFailure,"Stack storage exceeds container limits"});
    }
}
Result<Image> OperationStack::apply(ImageView source) const {
    auto result=evaluate(source);
    if (!result) return std::unexpected(result.error().cause);
    return std::move(*result);
}
} // namespace iiGraphicAdjust
