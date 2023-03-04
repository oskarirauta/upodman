#pragma once

#include <map>
#include <string>

#ifndef BUSY_COUNTDOWN_CYCLES
#define BUSY_COUNTDOWN_CYCLES 4
#endif

namespace Podman {

	class BusyStat {

		public:

			enum Value : uint8_t {
				NONE = 0,
				STARTING,
				STOPPING,
				RESTARTING,
				CREATING,
				REMOVING,
			};

			uint8_t count = 0;

			BusyStat() = default;
			constexpr BusyStat(Value aStat) : value(aStat) { }

			constexpr operator bool() const { return this -> value != Podman::BusyStat::NONE; }
			constexpr bool operator==(Podman::BusyStat a) const { return this -> value == a.value; }
			constexpr bool operator==(Podman::BusyStat::Value a) const { return this -> value == a; }
			constexpr bool operator!=(Podman::BusyStat a) const { return this -> value != a.value; }
			constexpr bool operator!=(Podman::BusyStat::Value a) const { return this -> value != a; }

			BusyStat& operator=(const Podman::BusyStat::Value& a) {
				this -> value = a;
				this -> count = a == Podman::BusyStat::Value::NONE ? 0 : BUSY_COUNTDOWN_CYCLES;
				return *this;
			}

			inline std::string description() const {
				if ( Podman::BusyStat::Description.count(this -> value))
					return Podman::BusyStat::Description.at(this -> value);
				else return "Unknown busy state description";
			}

			inline bool state(void) const {
				return this -> value != Podman::BusyStat::Value::NONE;
			}

			inline void reset(void) {
				this -> value = Podman::BusyStat::NONE;
				this -> count = 0;
			}

			inline void cycle(void) {

				if ( this -> count == 0 && this -> value == Podman::BusyStat::Value::NONE )
					return;
				else if ( this -> count != 0 && this -> value == Podman::BusyStat::Value::NONE )
					this -> count = 0;
				else if ( this -> count == 0 && this -> value != Podman::BusyStat::Value::NONE )
					this -> value = Podman::BusyStat::Value::NONE;
				else if ( this -> count > 1 && this -> value != Podman::BusyStat::Value::NONE ) 
					this -> count--;
				else if ( this -> count == 1 && this -> value != Podman::BusyStat::Value::NONE ) {
					this -> count = 0;
					this -> value = Podman::BusyStat::Value::NONE;
				}
			}

		private:

			Value value;

			static inline std::map<Podman::BusyStat::Value, std::string> Description = {
				{ NONE, "none" },
				{ STARTING, "starting" },
				{ STOPPING, "stopping" },
				{ RESTARTING, "restarting" },
				{ CREATING, "creating" },
				{ REMOVING, "removing" },
			};

	};

}
