#pragma once

#include <common/error.hpp>
#include <tensor/scalar.hpp>
#include <tensor/tensor.hpp>

namespace Construction {
	namespace Tensor {

		class Tensor;

        class InvalidSubstitutionException : public Exception {
        public:
            InvalidSubstitutionException() : Exception("The substitution is invalid") { }
        };

		class Substitution : public AbstractExpression {
		public:
			Substitution() = default;

			Substitution(const Scalar& variable, const Scalar& other) {
				substitutions.push_back({variable,other});
			}
		public:
			void Insert(const Scalar& variable, const Scalar& expression) {
				substitutions.push_back({variable, expression});
			}

			virtual bool IsSubstitutionExpression() const override { return true; }
			virtual inline int GetColorCode() const override { return 36; }
		public:
			inline Scalar operator()(const Scalar& scalar) const {
				Scalar result = scalar;
				for (auto& s : substitutions) {
					result = std::move(result.Substitute(s.first, s.second));
				}
				return result;
			}

			inline Tensor operator()(const Tensor& tensor) const {
				return tensor.SubstituteVariables(substitutions);
			}
		public:
			virtual ExpressionPointer Clone() const override {
				return ExpressionPointer(new Substitution(*this));
			}

			virtual std::string ToString() const override {
				std::stringstream ss;

				for (auto& s : substitutions) {
					ss << s.first << " = " << s.second << std::endl;
				}

				return ss.str();
			}
        public:
            std::vector< std::pair<Scalar, Scalar> >::iterator begin() { return substitutions.begin(); }
            std::vector< std::pair<Scalar, Scalar> >::iterator end() { return substitutions.end(); }

            std::vector< std::pair<Scalar, Scalar> >::const_iterator begin() const { return substitutions.begin(); }
            std::vector< std::pair<Scalar, Scalar> >::const_iterator end() const { return substitutions.end(); }
        public:
            /**
                \brief Merge multiple substitutions into one common

             */
            static Substitution Merge(const std::vector<Substitution> substitutions) {
                if (substitutions.size() == 1) return substitutions[0];

                // Turn this into a matrix
                std::vector<Scalar> variables;

                std::vector<std::unordered_map<Scalar, double>> data;

                // Iterate over all substitutions
                int i=0;
                int pos=0;
                for (auto& subst : substitutions) {
                    // Iterate over all scalars in the substitution
                    for (auto& s : subst) {
                        // Turn this into an equation of the form lhs-rhs = 0
                        Scalar eq = s.first;

                        auto summands = s.second.GetSummands();
                        for (auto& t : summands) {
                            eq -= t;
                        }

                        auto r = eq.SeparateVariablesFromRest();

                        std::unordered_map<Scalar, double> map;

                        bool first=true;

                        // Add the variable to the list
                        for (auto& v : r.first) {
                            if (first) {
                                first = false;

                                auto it = std::find(variables.begin(), variables.end(), v.first);
                                if (it != variables.end()) {
                                    variables.erase(it);
                                }

                                variables.insert(variables.begin() + pos++, v.first);
                            } else {
                                if (std::find(variables.begin(), variables.end(), v.first) == variables.end()) {
                                    variables.push_back(v.first);
                                }
                            }

                            if (!v.second.IsNumeric()) assert(false);

                            // Get the value
                            map[v.first] = v.second.ToDouble();
                        }

                        data.push_back(map);
                    }
                    i++;
                }

                // Write the elements into a matrix
                Vector::Matrix M(data.size(), variables.size());

                // Insert the data from above
                for (int i=0; i<data.size(); ++i) {
                    for (int j=0; j<variables.size(); ++j) {
                        M(i,j) = data[i][variables[j]];
                    }
                }

                // Free memory in data
                data.clear();

                // Row reduce
                M.ToRowEchelonForm();

                // Read out
                Substitution result;

                // Extract the results
                for (int i=0; i<M.GetNumberOfRows(); i++) {
                    auto vec = M.GetRowVector(i);

                    // If the vector has zero norm, we get no further information => quit
                    if (vec * vec == 0) break;

                    bool isZero = true;
                    Scalar lhs = 0;
                    Scalar rhs = 0;

                    // Iterate over all the components
                    for (int j=0; j<vec.GetDimension(); j++) {
                        if (vec[j] == 0 && isZero) continue;
                        if (vec[j] == 1 && isZero) {
                            lhs = variables[j];
                            isZero = false;
                        } else if (vec[j] != 0) {
                            rhs += (-variables[j] * Scalar::Fraction(vec[j]));
                        }
                    }

                    // If the left hand side is not a variable, throw exception
                    if (lhs.IsNumeric() && lhs.ToDouble() == 0) {
                        throw InvalidSubstitutionException();
                    }

                    // Add to the result
                    result.Insert(lhs, rhs);
                }

                return result;
            }
		public:
			virtual void Serialize(std::ostream& os) const override {
				// Write size
				WriteBinary<size_t>(os, substitutions.size());

				// Write the pairs
				for (auto& substitution : substitutions) {
					substitution.first.Serialize(os);
					substitution.second.Serialize(os);
				}
			}

			static std::unique_ptr<AbstractExpression> Deserialize(std::istream& is) {
				// Read size
				size_t size = ReadBinary<size_t>(is);

				std::unique_ptr<Substitution> result (new Substitution());

				// Iterate over the entries
				for (int i=0; i<size; i++) {
					auto lhs = Scalar::Deserialize(is);
					if (!lhs) return nullptr;

					auto rhs = Scalar::Deserialize(is);
					if (!rhs) return nullptr;

					result->Insert(*static_cast<Scalar*>(lhs.get()), *static_cast<Scalar*>(rhs.get()));
				}

				return std::move(result);
			}
		private:
			std::vector< std::pair<Scalar, Scalar> > substitutions;
		};

	}
}
