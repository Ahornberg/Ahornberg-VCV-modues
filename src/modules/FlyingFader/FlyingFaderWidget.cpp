#include "FlyingFaderWidget.hpp"

const FaderCapColor FlyingFaderWidget::FADER_CAP_COLORS[] = {
	{ 0, "white" },
	{ 1, "grey" },
	{ 2, "black" },
	{ 3, "red" },
	{ 4, "blue" },
	{ 5, "green" },
	{ 6, "brown" },
	{ 7, "orange" },
	{ 8, "pink" },
	{ 9, "purple" }
};

MotorizedFader::MotorizedFader() {
	displayContextMenu = false;
	minHandlePos = Vec(0, 230);
	maxHandlePos = Vec(0, 2);
	setBackgroundSvg("res/knobs/MotorizedFaderBackground.svg");
	fb->dirty = true;
}

void MotorizedFader::setFlyingFader(FlyingFader* flyingFader) {
	this->flyingFader = flyingFader;
	if (flyingFader) {
		setHandleSvg("res/knobs/MotorizedFaderHandle_white.svg");
	} else {
		setHandleSvg("res/knobs/MotorizedFaderHandle_" + FlyingFaderWidget::FADER_CAP_COLORS[(int) (random::uniform() * FlyingFaderWidget::NUM_FADER_CAP_COLORS)].color + ".svg");
		handle->box.pos = Vec(0, 68.5);
	}
	fb->dirty = true;
}

void MotorizedFader::onButton(const event::Button& e) {
	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && (e.mods & RACK_MOD_MASK) == 0) {
		if (flyingFader) {
			flyingFader->faderDragged = true;
			displayContextMenu = true;
			// oldValue = paramQuantity->getValue();
		}
	}
	ParamWidget::onButton(e);
	
}

void MotorizedFader::onDragStart(const event::DragStart& e) {
	if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
		APP->window->cursorLock();
		if (paramQuantity && flyingFader) {
			oldValue = paramQuantity->getSmoothValue();
			oldFaderValueBeforeConnected = flyingFader->params[FlyingFader::FADER_VALUE_BEFORE_CONNECTED].getValue();
			flyingFader->faderDragged = true;
		}
	}
}

void MotorizedFader::onDragEnd(const event::DragEnd& e) {
	if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
		if (paramQuantity && flyingFader) {
			flyingFader->faderDragged = false;
			float newValue = paramQuantity->getSmoothValue();
			float newFaderValueBeforeConnected = flyingFader->params[FlyingFader::FADER_VALUE_BEFORE_CONNECTED].getValue();
			if (oldValue != newValue) {
				// Push ParamChange history action
				history::ComplexAction* complexAction = new history::ComplexAction;
				complexAction->name = "move slider";
				history::ParamChange* oldFaderValue = new history::ParamChange;
				oldFaderValue->name = "move slider";
				oldFaderValue->moduleId = paramQuantity->module->id;
				oldFaderValue->paramId = FlyingFader::FADER_VALUE_BEFORE_CONNECTED;
				oldFaderValue->oldValue = oldFaderValueBeforeConnected;
				oldFaderValue->newValue = newFaderValueBeforeConnected;
				complexAction->push(oldFaderValue);
				history::ParamChange* faderMove = new history::ParamChange;
				faderMove->name = "move slider";
				faderMove->moduleId = paramQuantity->module->id;
				faderMove->paramId = paramQuantity->paramId;
				faderMove->oldValue = oldValue;
				faderMove->newValue = newValue;
				complexAction->push(faderMove);
				APP->history->push(complexAction);
			}
		}
		APP->window->cursorUnlock();
	}
}

void MotorizedFader::step() {
	if (flyingFader && displayContextMenu) {
		MenuOverlay* overlay = NULL;
		for (Widget* child : APP->scene->children) {
			overlay = dynamic_cast<MenuOverlay*>(child);
			if (overlay) break;
		}
		if (!overlay) {
			flyingFader->faderDragged = false;
			displayContextMenu = false;
		}
	}
	SvgSlider::step();
}

void TextOnFaderModule::drawText(const Widget::DrawArgs& disp, Rect box) {
	nvgBeginPath(disp.vg);
	nvgRotate(disp.vg, (-90 * NVG_PI) / 180);
	if (useScissor) {
		nvgScissor(disp.vg, box.size.x * -0.5, 0, box.size.x, box.size.y);
	}
	nvgFontSize(disp.vg, fontSize);
	nvgFontFaceId(disp.vg, font->handle);
	nvgFillColor(disp.vg, textColor);
	nvgTextAlign(disp.vg, textAlign);
	nvgText(disp.vg, textPos.x, textPos.y, text.c_str(), NULL);
}

FaderNameDisplay::FaderNameDisplay(Rect box) : SizedTransparentWidget(box) {
	font = APP->window->loadFont(asset::plugin(pluginInstance, FONT_HANDWRITE));
	textColor = COLOR_BLACK;
	text = FlyingFader::INIT_FADER_NAME;
	fontSize = 16;
	textAlign = NVG_ALIGN_CENTER;
	useScissor = true;
	textPos =  Vec(0, 11);
}

void FaderNameDisplay::draw(const DrawArgs& disp) {
	drawText(disp, box);
}

FaderNameMenuItem::FaderNameMenuItem(FaderNameDisplay* faderNameDisplay) {
	this->faderNameDisplay = faderNameDisplay;
	text = faderNameDisplay->text;
}

void FaderNameMenuItem::onChange(const event::Change& e) {
	TextFieldMenuItem::onChange(e);
	faderNameDisplay->text = text;
}

FlyingFaderWidget::FlyingFaderWidget(FlyingFader* module) {
	setModule(module);
	setPanel("res/FlyingFader.svg");
	setSize(Vec(60, 380));
	setScrews(SCREW_TOP_LEFT, NO_SCREW_TOP_RIGHT, NO_SCREW_BOTTOM_LEFT, SCREW_BOTTOM_RIGHT);
	faderCapColorIndex = 0;

	faderNameDisplay = new FaderNameDisplay(Rect(6.5, 214.5, 153, 18));
	addChild(faderNameDisplay);

	fader = dynamic_cast<MotorizedFader*>(createParam<MotorizedFader>(Vec(18, 40.5), module, FlyingFader::FADER_PARAM));
	fader->setFlyingFader(module);
	addParam(fader);

	addInput(createInputCentered<InPort>(Vec(15.5, 357), module, FlyingFader::CV_INPUT));
	addInput(createInputCentered<InPort>(Vec(44.5, 349.5), module, FlyingFader::AUDIO_INPUT));
	
	addOutput(createOutputCentered<OutPort>(Vec(15.5, 330), module, FlyingFader::CV_OUTPUT));
	addOutput(createOutputCentered<OutPort>(Vec(44.5, 23), module, FlyingFader::AUDIO_OUTPUT));
}

void FlyingFaderWidget::appendContextMenu(Menu* menu) {
	// TapeRecorder* tapeRecorder = dynamic_cast<TapeRecorder*>(this->module);
	menu->addChild(new MenuEntry);
	menu->addChild(new FaderNameMenuItem(faderNameDisplay));
	menu->addChild(new FaderCapColorMenuItem(this, faderCapColorIndex));
}

void FlyingFaderWidget::changeFaderCapColor(int faderCapColorIndex) {
	fader->setHandleSvg("res/knobs/MotorizedFaderHandle_" + FADER_CAP_COLORS[faderCapColorIndex].color + ".svg");
	event::Change eChange;
	fader->onChange(eChange);
	this->faderCapColorIndex = faderCapColorIndex;
}

json_t* FlyingFaderWidget::toJson() {
	json_t* rootJ = ModuleWidget::toJson();
	
	json_object_set_new(rootJ, "fader-name", json_string(faderNameDisplay->text.c_str()));
	json_object_set_new(rootJ, "fader-cap-color", json_integer(faderCapColorIndex));
	return rootJ;
}

void FlyingFaderWidget::fromJson(json_t* rootJ) {
	ModuleWidget::fromJson(rootJ);
	
	json_t* faderNameJ = json_object_get(rootJ, "fader-name");
	if (faderNameJ) {
		faderNameDisplay->text = json_string_value(faderNameJ);
	}
	json_t* faderCapColorJ = json_object_get(rootJ, "fader-cap-color");
	if (faderCapColorJ) {
		changeFaderCapColor(json_integer_value(faderCapColorJ));
		// faderCapColorIndex = json_integer_value(faderCapColorJ);
	}
	// if (module) {
		// dynamic_cast<FlyingFader*>(this->module)->init();
	// }
}

FaderCapColorValueItem::FaderCapColorValueItem(FlyingFaderWidget* flyingFaderWidget, int faderCapColorIndex) {
	this->flyingFaderWidget = flyingFaderWidget;
	this->faderCapColorIndex = faderCapColorIndex;
	text = FlyingFaderWidget::FADER_CAP_COLORS[faderCapColorIndex].color;
	rightText = CHECKMARK(FlyingFaderWidget::FADER_CAP_COLORS[faderCapColorIndex].index == flyingFaderWidget->faderCapColorIndex);
}

void FaderCapColorValueItem::onAction(const event::Action& e) {
	flyingFaderWidget->changeFaderCapColor(faderCapColorIndex);
}

FaderCapColorMenuItem::FaderCapColorMenuItem(FlyingFaderWidget* flyingFaderWidget, int faderCapColorIndex) {
	this->flyingFaderWidget = flyingFaderWidget;
	this->faderCapColorIndex = faderCapColorIndex;
	text = "Fader Cap Color";
	rightText = FlyingFaderWidget::FADER_CAP_COLORS[faderCapColorIndex].color + " " + RIGHT_ARROW;
}

Menu* FaderCapColorMenuItem::createChildMenu() {
	Menu* menu = new Menu;
	for (auto i = 0; i < FlyingFaderWidget::NUM_FADER_CAP_COLORS; ++i) {
		menu->addChild(new FaderCapColorValueItem(flyingFaderWidget, i));
	}
	return menu;
}

Model* modelFlyingFader = createModel<FlyingFader, FlyingFaderWidget>("FlyingFader");
