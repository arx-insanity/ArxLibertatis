/*
 * Copyright 2011-2021 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Based on:
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#include "script/ScriptedVariable.h"

#include <cstring>
#include <string>
#include <string_view>

#include "game/Entity.h"
#include "graphics/data/Mesh.h"
#include "script/ScriptEvent.h"
#include "script/ScriptUtils.h"


namespace script {

namespace {

class SetCommand : public Command {
	
public:
	
	SetCommand() : Command("set") { }
	
	Result execute(Context & context) override {
		
		std::string var = context.getWord();
		std::string val = context.getWord();
		
		DebugScript(' ' << var << " \"" << val << '"');
		
		if(var.empty()) {
			ScriptWarning << "Missing variable name";
			return Failed;
		}
		
		SCRIPT_VARIABLES & variables = isLocalVariable(var) ? context.getEntity()->m_variables : svar;
		
		SCRIPT_VAR * sv = nullptr;
		switch(var[0]) {
			
			case '$':      // global text
			case '\xA3': { // local text
				sv = SETVarValueText(variables, var, context.getStringVar(val));
				break;
			}
			
			case '#':      // global long
			case '\xA7': { // local long
				sv = SETVarValueLong(variables, var, long(context.getFloatVar(val)));
				break;
			}
			
			case '&':      // global float
			case '@': {    // local float
				sv = SETVarValueFloat(variables, var, context.getFloatVar(val));
				break;
			}
			
			default: {
				ScriptWarning << "Unknown variable type: " << var;
				return Failed;
			}
			
		}
		
		if(!sv) {
			ScriptWarning << "Unable to set variable " << var;
			return Failed;
		}
		
		return Success;
	}
	
};

class ArithmeticCommand : public Command {
	
public:
	
	enum Operator {
		Add,
		Subtract,
		Multiply,
		Divide
	};
	
private:
	
	float calculate(float left, float right) {
		switch(op) {
			case Add: return left + right;
			case Subtract: return left - right;
			case Multiply: return left * right;
			case Divide: return (right == 0.f) ? 0.f : left / right;
		}
		arx_assert_msg(false, "Invalid op used in ArithmeticCommand: %d", int(op));
		return 0.f;
	}
	
	Operator op;
	
public:
	
	ArithmeticCommand(std::string_view name, Operator _op) : Command(name), op(_op) { }
	
	Result execute(Context & context) override {
		
		std::string var = context.getWord();
		float val = context.getFloat();
		
		DebugScript(' ' << var << ' ' << val);
		
		if(var.empty()) {
			ScriptWarning << "Missing variable name";
			return Failed;
		}
		
		SCRIPT_VARIABLES & variables = isLocalVariable(var) ? context.getEntity()->m_variables : svar;
		
		SCRIPT_VAR * sv = nullptr;
		switch(var[0]) {
			
			case '$':      // global text
			case '\xA3': { // local text
				ScriptWarning << "Cannot calculate with text variables";
				return Failed;
			}
			
			case '#':      // global long
			case '\xA7': { // local long
				long old = GETVarValueLong(variables, var);
				sv = SETVarValueLong(variables, var, long(calculate(float(old), val)));
				break;
			}
			
			case '&':   // global float
			case '@': { // local float
				float old = GETVarValueFloat(variables, var);
				sv = SETVarValueFloat(variables, var, calculate(old, val));
				break;
			}
			
			default: {
				ScriptWarning << "Unknown variable type: " << var;
				return Failed;
			}
			
		}
		
		if(!sv) {
			ScriptWarning << "Unable to set variable " << var;
			return Failed;
		}
		
		return Success;
	}
	
};

class UnsetCommand : public Command {
	
	// TODO move to variable context
	static void UNSETVar(SCRIPT_VARIABLES & svf, std::string_view name) {
		
		SCRIPT_VARIABLES::iterator it;
		for(it = svf.begin(); it != svf.end(); ++it) {
			if(it->name == name) {
				svf.erase(it);
				break;
			}
		}
		
	}
	
public:
	
	UnsetCommand() : Command("unset") { }
	
	Result execute(Context & context) override {
		
		std::string var = context.getWord();
		
		DebugScript(' ' << var);
		
		if(var.empty()) {
			ScriptWarning << "missing variable name";
			return Failed;
		}
		
		SCRIPT_VARIABLES & variables = isLocalVariable(var) ? context.getEntity()->m_variables : svar;
		
		UNSETVar(variables, var);
		
		return Success;
	}
	
};

class IncrementCommand : public Command {
	
	long m_diff;
	
public:
	
	IncrementCommand(std::string_view name, long diff) : Command(name), m_diff(diff) { }
	
	Result execute(Context & context) override {
		
		std::string var = context.getWord();
		
		DebugScript(' ' << var);
		
		if(var.empty()) {
			ScriptWarning << "missing variable name";
			return Failed;
		}
		
		SCRIPT_VARIABLES & variables = isLocalVariable(var) ? context.getEntity()->m_variables : svar;
		
		SCRIPT_VAR * sv = nullptr;
		switch(var[0]) {
			
			case '$':
			case '\xA3': {
				ScriptWarning << "Cannot increment text variables";
				return Failed;
			}
			
			case '#':
			case '\xA7': {
				sv = SETVarValueLong(variables, var, GETVarValueLong(variables, var) + m_diff);
				break;
			}
			
			case '&':
			case '@': {
				sv = SETVarValueFloat(variables, var, GETVarValueFloat(variables, var) + float(m_diff));
				break;
			}
			
			default: {
				ScriptWarning << "Unknown variable type: " << var;
				return Failed;
			}
			
		}
		
		if(!sv) {
			ScriptWarning << "Unable to set variable " << var;
			return Failed;
		}
		
		return Success;
	}
	
};

class PushCommand : public Command {
public:
	PushCommand() : Command("push") { }

	Result execute(Context & context) override {
		std::string value = context.getWord();
		std::string separator = context.getWord();
		std::string list = context.getWord();

		DebugScript(' ' << value << ' ' << separator << ' ' << list);

		if (value.empty()) {
			ScriptWarning << "missing variable name for value";
			return Failed;
		}

		if (separator != "to") {
			ScriptWarning << "expected 'to'";
			return Failed;
		}

		if (list.empty()) {
			ScriptWarning << "missing variable name for space separated string";
			return Failed;
		}

		// TODO

		return Success;
	}
};

class PopCommand : public Command {
public:
	PopCommand() : Command("pop") { }

	Result execute(Context & context) override {
		std::string value = context.getWord();
		std::string separator = context.getWord();
		std::string list = context.getWord();

		DebugScript(' ' << value << ' ' << separator << ' ' << list);

		if (value.empty()) {
			ScriptWarning << "missing variable name for value";
			return Failed;
		}

		if (separator != "from") {
			ScriptWarning << "expected 'from'";
			return Failed;
		}

		if (list.empty()) {
			ScriptWarning << "missing variable name for space separated string";
			return Failed;
		}

		// TODO

		return Success;
	}
};

class ShiftCommand : public Command {
public:
	ShiftCommand() : Command("shift") { }

	Result execute(Context & context) override {
		std::string value = context.getWord();
		std::string separator = context.getWord();
		std::string list = context.getWord();

		DebugScript(' ' << value << ' ' << separator << ' ' << list);

		if (value.empty()) {
			ScriptWarning << "missing variable name for value";
			return Failed;
		}

		if (separator != "from") {
			ScriptWarning << "expected 'from'";
			return Failed;
		}

		if (list.empty()) {
			ScriptWarning << "missing variable name for space separated string";
			return Failed;
		}

		// TODO

		return Success;
	}
};

class UnshiftCommand : public Command {
public:
	UnshiftCommand() : Command("unshift") { }

	Result execute(Context & context) override {
		std::string value = context.getWord();
		std::string separator = context.getWord();
		std::string list = context.getWord();

		DebugScript(' ' << value << ' ' << separator << ' ' << list);

		if (value.empty()) {
			ScriptWarning << "missing variable name for value";
			return Failed;
		}

		if (separator != "to") {
			ScriptWarning << "expected 'to'";
			return Failed;
		}

		if (list.empty()) {
			ScriptWarning << "missing variable name for space separated string";
			return Failed;
		}

		// TODO

		return Success;
	}
};

} // anonymous namespace

void setupScriptedVariable() {
	
	ScriptEvent::registerCommand(std::make_unique<SetCommand>());
	ScriptEvent::registerCommand(std::make_unique<ArithmeticCommand>("inc", ArithmeticCommand::Add));
	ScriptEvent::registerCommand(std::make_unique<ArithmeticCommand>("dec", ArithmeticCommand::Subtract));
	ScriptEvent::registerCommand(std::make_unique<ArithmeticCommand>("mul", ArithmeticCommand::Multiply));
	ScriptEvent::registerCommand(std::make_unique<ArithmeticCommand>("div", ArithmeticCommand::Divide));
	ScriptEvent::registerCommand(std::make_unique<UnsetCommand>());
	ScriptEvent::registerCommand(std::make_unique<IncrementCommand>("++", 1));
	ScriptEvent::registerCommand(std::make_unique<IncrementCommand>("--", -1));
	ScriptEvent::registerCommand(std::make_unique<PushCommand>());
	ScriptEvent::registerCommand(std::make_unique<PopCommand>());
	ScriptEvent::registerCommand(std::make_unique<UnshiftCommand>());
	ScriptEvent::registerCommand(std::make_unique<ShiftCommand>());
}

} // namespace script
