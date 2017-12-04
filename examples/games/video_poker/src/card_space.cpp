﻿/*
neogfx C++ GUI Library - Examples - Games - Video Poker
Copyright(C) 2017 Leigh Johnston

This program is free software: you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <neogfx/neogfx.hpp>
#include <video_poker/card_space.hpp>

namespace video_poker
{
	namespace
	{
		const neogfx::size kBridgeCardSize{ 57.15, 88.9 };
	}

	card_widget::card_widget(neogfx::i_layout& aLayout, const i_card_textures& aCardTextures) :
		widget{ aLayout },
		iCardTextures{ aCardTextures },
		iCard{ nullptr }
	{
		set_margins(neogfx::margins{});
		set_size_policy(neogfx::size_policy::Expanding, kBridgeCardSize);
	}

	neogfx::size card_widget::minimum_size(const neogfx::optional_size& aAvailableSpace) const
	{
		neogfx::scoped_units su(*this, neogfx::units::Millimetres);
		auto minSize = convert_units(*this, neogfx::units::Pixels, kBridgeCardSize * 0.5);
		su.restore_saved_units();
		minSize = neogfx::units_converter(*this).from_device_units(minSize);
		return minSize.ceil();
	}

	neogfx::size card_widget::maximum_size(const neogfx::optional_size& aAvailableSpace) const
	{
		return (minimum_size(aAvailableSpace) / 0.5).ceil();
	}

	void card_widget::mouse_button_pressed(neogfx::mouse_button aButton, const neogfx::point& aPosition, neogfx::key_modifiers_e aKeyModifiers)
	{
		neogfx::widget::mouse_button_pressed(aButton, aPosition, aKeyModifiers);
		toggle_hold();
	}

	void card_widget::mouse_button_double_clicked(neogfx::mouse_button aButton, const neogfx::point& aPosition, neogfx::key_modifiers_e aKeyModifiers)
	{
		neogfx::widget::mouse_button_double_clicked(aButton, aPosition, aKeyModifiers);
		toggle_hold();
	}
		
	void card_widget::paint(neogfx::graphics_context& aGraphicsContext) const
	{
		auto rect = client_rect();
		aGraphicsContext.fill_rounded_rect(rect, rect.cx / 10.0, neogfx::colour::DarkGreen);
		rect.deflate(neogfx::size{ 4.0 });
		aGraphicsContext.fill_rounded_rect(rect, rect.cx / 10.0, background_colour());
		rect.inflate(neogfx::size{ 4.0 });

		if (has_card())
		{
			// todo: use a sprite to render card
			aGraphicsContext.fill_rounded_rect(rect, rect.cx / 10.0, neogfx::colour::White);
			if (card().is_face_card())
			{
				auto faceRect = rect;
				faceRect.deflate(neogfx::size{ 9.0 });
				aGraphicsContext.draw_texture(faceRect, iCardTextures.face_texture(card()), neogfx::colour::AliceBlue, neogfx::shader_effect::Colourize);
			}
			const neogfx::colour suitColour = (card() == card::suit::Diamond || card() == card::suit::Heart ?
				neogfx::colour{ 213, 0, 0 } : neogfx::colour::Black);
			const auto& valueTexture = iCardTextures.value_texture(card());
			aGraphicsContext.draw_texture(client_rect().centre() - (valueTexture.extents() / 2.0) + neogfx::point{ 0.0, -18.0 }, valueTexture, suitColour);
			const auto& suitTexture = iCardTextures.suit_texture(card());
			aGraphicsContext.draw_texture(neogfx::rect{ (client_rect().centre() - (neogfx::size{36.0} / 2.0)) + neogfx::point{ 0.0, 48.0 } + neogfx::point{ 0.0, -18.0 }, neogfx::size{36.0} }, suitTexture);
		}
	}

	bool card_widget::has_card() const
	{
		return iCard != nullptr;
	}

	card& card_widget::card() const
	{
		return *iCard;
	}

	void card_widget::set_card(video_poker::card& aCard)
	{
		iCard = &aCard;
		iSink += iCard->changed([this](video_poker::card&) { update(); });
		update();
	}

	void card_widget::clear_card()
	{
		iCard = nullptr;
		update();
	}

	void card_widget::toggle_hold()
	{
		if (has_card())
		{
			if (!card().discarded())
				card().discard();
			else
				card().undiscard();
		}
	}

	card_space::card_space(neogfx::i_layout& aLayout, neogfx::sprite_plane& aSpritePlane, i_table& aTable) :
		widget{ aLayout },
		iSpritePlane{ aSpritePlane }, 
		iTable{ aTable },
		iVerticalLayout{ *this, neogfx::alignment::Centre | neogfx::alignment::VCentre },
		iCardWidget{ iVerticalLayout, aTable.textures() }, 
		iHoldButton{ iVerticalLayout, u8"HOLD\n CANCEL " },
		iCard{ nullptr }
	{
		set_size_policy(neogfx::size_policy::ExpandingNoBits);
		iVerticalLayout.set_spacing(neogfx::size{ 8.0 });
		iHoldButton.set_size_policy(neogfx::size_policy::Minimum);
		iHoldButton.set_foreground_colour(neogfx::color::Black);
		iHoldButton.text().set_font(neogfx::font{ "Exo 2", "Black", 16.0 });
		iHoldButton.text().set_text_appearance(neogfx::text_appearance{ neogfx::color::White, neogfx::text_effect{ neogfx::text_effect::Outline, neogfx::colour::Black.with_alpha(128) } });
		iHoldButton.set_checkable();
		auto update_hold = [this]() 
		{ 
			if (has_card())
			{
				if (iHoldButton.is_checked())
					card().undiscard();
				else
					card().discard();
				update_widgets();
			}
		};
		iHoldButton.toggled(update_hold);
		iTable.state_changed([this](table_state) { update_widgets(); });
		update_widgets();
	}

	bool card_space::has_card() const
	{
		return iCard != nullptr;
	}

	const video_poker::card& card_space::card() const
	{
		if (has_card())
			return *iCard;
		throw no_card();
	}

	video_poker::card& card_space::card()
	{
		if (has_card())
			return *iCard;
		throw no_card();
	}

	void card_space::set_card(video_poker::card& aCard)
	{
		iCard = &aCard;
		iCardWidget.set_card(card());
		iSink += card().changed([this](video_poker::card&) { update_widgets(); });
		iSink += card().destroyed([this](video_poker::card&) { clear_card(); });
		update_widgets();
	}

	void card_space::clear_card()
	{
		iCard = nullptr;
		iCardWidget.clear_card();
		update_widgets();
	}

	void card_space::update_widgets()
	{
		iCardWidget.enable(has_card() && iTable.state() == table_state::DealtFirst);
		iHoldButton.set_foreground_colour(has_card() && !card().discarded() && iTable.state() == table_state::DealtFirst ? neogfx::colour::LightYellow1 : neogfx::colour::Black.with_alpha(128));
		iHoldButton.enable(has_card() && iTable.state() == table_state::DealtFirst);
		iHoldButton.set_checked(has_card() && !card().discarded() && iTable.state() == table_state::DealtFirst);
	}
}