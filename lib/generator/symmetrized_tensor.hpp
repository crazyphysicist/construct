#pragma once

#include <common/time_measurement.hpp>

#include <tensor/tensor.hpp>
#include <tensor/tensor_container.hpp>
#include <tensor/symmetrization.hpp>

#include <generator/equivalent_selector.hpp>

using Construction::Tensor::Symmetrization;
using Construction::Tensor::AntiSymmetrization;
using Construction::Tensor::BlockSymmetrization;

namespace Construction {
    namespace Generator {

        using Tensor::Indices;

        /**
            \class Symmetry


         */
        class Symmetry {
        public:
            enum class SymmetryType {
                SYMMETRY = 101,
                ANTISYMMETRY = 102,
                BLOCKSYMMETRY = 103
            };
        public:
            Symmetry(const std::vector<Tensor::Indices>& blocks) : blocks(blocks) {
                if (blocks.size() > 1) {
                    type = SymmetryType::BLOCKSYMMETRY;
                }
            }

            Symmetry(const Tensor::Indices& indices) {
                blocks.push_back(indices);
            }

            Symmetry(const Symmetry& other) : type(other.type), blocks(other.blocks) { }
            Symmetry(Symmetry&& other) : type(std::move(other.type)), blocks(std::move(other.blocks)) { }
        public:
            bool IsSymmetric() const { return type == SymmetryType::SYMMETRY; }
            bool IsAntiSymmetric() const { return type == SymmetryType::ANTISYMMETRY; }
            bool IsBlockSymmetric() const { return type == SymmetryType::BLOCKSYMMETRY; }
        private:
            SymmetryType type = SymmetryType::SYMMETRY;
            std::vector<Tensor::Indices> blocks;
        };

        class SymmetrizedTensorGenerator {
        public:
            SymmetrizedTensorGenerator(const Indices& symmetrization) : symmetrization(symmetrization) { }
        public:
            TensorContainer operator()(const TensorContainer& tensors, bool scaledResult=true) const {
                TensorContainer result;

                // Symmetrize everything in the list
                for (auto& tensor : tensors) {
                    Common::TimeMeasurement time;

                    // Build symmetrization
                    std::vector<unsigned> s;
                    for (auto& index : symmetrization) {
                        int pos = tensor->GetIndices().IndexOf(index) + 1;
                        s.push_back(pos);
                    }

                    Symmetrization symm(s, scaledResult);

                    // Symmetrize
                    auto newTensor = std::move(symm(tensor));

                    // Check if the tensor is zero
                    if (!newTensor->IsZero()) {

                    result.Insert(newTensor);

                    //    result.Insert(newTensor->Clone());
                    //    result.Insert(std::move(newTensor));
                    }
                }

                //Symmetrization symm(s, scaledResult);

                EquivalentSelector selector;
                return selector(result);
            }
        private:
            Indices symmetrization;
        };

        class AntiSymmetrizedTensorGenerator {
        public:
            AntiSymmetrizedTensorGenerator(const Indices& symmetrization) : symmetrization(symmetrization) { }
        public:
            TensorContainer operator()(const TensorContainer& tensors, bool scaledResult=true) const {
                TensorContainer result;

                // Symmetrize everything in the list
                for (auto& tensor : tensors) {
                    // Build symmetrization
                    std::vector<unsigned> s;
                    for (auto& index : symmetrization) {
                        int pos = tensor->GetIndices().IndexOf(index) + 1;
                        s.push_back(pos);
                    }
                    AntiSymmetrization symm(s, scaledResult);

                    // Symmetrize
                    auto newTensor = symm(tensor);

                    // Check if the tensor is zero
                    if (!newTensor->IsZero()) result.Insert(std::move(newTensor));
                }

                EquivalentSelector selector;
                return selector(result);
            }
        private:
            Indices symmetrization;
        };

        class ExchangeSymmetrizedTensorGenerator {
        public:
            ExchangeSymmetrizedTensorGenerator(const Indices& indices) : indices(indices) { }
        public:
            TensorContainer operator()(const TensorContainer& tensors, bool scaledResult=false) const {
                TensorContainer result;

                for (auto& tensor : tensors) {
                    auto copyTensor = tensor->Clone();
                    copyTensor->SetIndices(indices);

                    auto added = std::make_shared<Tensor::AddedTensor>(tensor, std::move(copyTensor));
                    auto scaled = std::make_shared<Tensor::ScaledTensor>(added, 0.5);

                    if ((0.5*(*added)).IsEqual(*tensor)) {
                        result.Insert(std::move(tensor->Clone()));
                        continue;
                    }

                    if (!scaledResult)
                        result.Insert(std::move(added));
                    else result.Insert(std::make_shared<Tensor::ScaledTensor>(std::move(added), 0.5));
                }

                EquivalentSelector selector;
                return selector(result);
            }
        private:
            Indices indices;
        };

        class BlockSymmetrizedTensorGenerator {
        public:
            BlockSymmetrizedTensorGenerator(const std::vector<Indices>& blocks) : blocks(blocks) { Validate(); }
        public:
            void Validate() const {
                // If no indices in there, symmetrization is trivial, therefore return true
                if (blocks.size() == 0) return;

                // Get the number of indices for the first element
                auto size = blocks[0].Size();

                // Iterate over all index blocks and check if the sizes match
                for (auto& indices : blocks) {
                    if (indices.Size() != size) {
                        throw Exception("Block symmetrization can only go over indices of same length");
                    }
                }
            }

            TensorContainer operator()(const TensorContainer& tensors, bool scaledResult=false) const {
                TensorContainer result;

                if (tensors.Size()== 0) return result;

                // Build symmetrization
                std::vector<std::pair<unsigned, unsigned>> s;

                for (auto& block : blocks) {
                    int first = tensors.Get(0)->GetIndices().IndexOf(block[0]) + 1;
                    int next = first + 1;
                    for (int j=1; j<block.Size(); j++) {
                        if (tensors.Get(0)->GetIndices().IndexOf(block[j])+1 != next) {
                            throw Exception("You need to specify blocks over neighbooring indices");
                        }
                        next++;
                    }
                    s.push_back({first, next-1});
                }
                BlockSymmetrization symm(s, scaledResult);

                // Symmetrize everything in the list
                for (auto& tensor : tensors) {
                    // Symmetrize
                    auto newTensor = std::move(symm(tensor));

                    // Check if the tensor is zero
                    if (!newTensor->IsZero()) {
                        result.Insert(newTensor);
                    }
                }

                EquivalentSelector selector;
                return selector(result);
            }
        private:
            std::vector<Indices> blocks;
        };

    }
}
