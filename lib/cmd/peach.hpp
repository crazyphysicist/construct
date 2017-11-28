#pragma once

#include <cobalt.hpp>

#include <solve_peach.hpp>

namespace Peach {
    namespace Cmd {
    
        class RootCommand : public Cobalt::Command<RootCommand, SolveCommand>
        {
        public:
            static std::string Use() {
                return "peach";
            }
            
            static std::string Short() {
                return "Solve the perturbative covariant gravitational closure equations";
            }

            static std::string Long() {
                return "Solve the perturbative covariant gravitational closure equations.";
            }

            void RegisterFlags() {
                AddPersistentFlag<bool>(debugMode, "debug", "d", false, "Print everything that is happening");
            }
            
            void PersistentPreRun(const Cobalt::Arguments& args) {
                std::cerr << "The infamous Peach Program" << std::endl;
                std::cerr << "(c) 2017 Constructive Gravity Group Erlangen" << std::endl;
                std::cerr << "All rights reserved." << std::endl << std::endl;
            }
        private:
            bool debugMode;
        };
    
    } /* namespace Cmd */
    
    // Syntactic sugar
    using Execute = Cobalt::Execute<Cmd::RootCommand>;
    
} /* namespace Peach */
