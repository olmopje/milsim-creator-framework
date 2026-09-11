//! A map of the board's own, and nobody else's.
//!
//! WHY THIS EXISTS. SCR_MapEntity is a singleton: one static instance, one
//! open flag, one set of invokers, one frame, one zoom. A board that shares
//! it is always in somebody's way -- hold it open and the player's M does the
//! wrong thing, hand it back and the board goes blank, re-state its view and
//! the board is still quietly writing to the thing a player is reading.
//!
//! But the singleton is SCR_MapEntity, NOT the class it inherits.
//! MapEntity is a plain GenericEntity with the whole map on it:
//! AbsorbData, InitializeLayers, SetLayer, EnableVisualisation, SetFrame,
//! ZoomChange, PosChange, EnableGrid. Nothing in that list is static and
//! nothing in it asks whether a map is "open" -- that word only exists in
//! SCR_MapEntity's bookkeeping.
//!
//! So the board spawns one of these and drives it directly. It borrows its
//! numbers from the real map entity once, at setup -- the layer
//! configuration, the fitted zoom, the pan that centres the island -- and
//! from then on it is its own map: its own view, changed by whoever walks up
//! to the board, and invisible to every player's own map.
//!
//! THE ONE THING THIS CANNOT ANSWER FROM THE SOURCE is whether a MapWidget
//! draws from a PARTICULAR MapEntity or simply from whichever one last set
//! the native state. There is no API that binds a widget to an entity, which
//! is exactly why it has to be tried rather than reasoned about. If the board
//! draws while a player pans their own map somewhere else, this works and the
//! board is independent. If it goes blank or follows theirs, it does not, and
//! MCF_Map_BoardComponent.m_bOwnMapEntity turns it off again.
[EntityEditorProps(category: "MCF/Ops", description: "A map entity belonging to one MCF map board. Spawned by MCF_Map_BoardComponent, never placed by hand.")]
class MCF_Map_BoardEntityClass : MapEntityClass
{
}

class MCF_Map_BoardEntity : MapEntity
{
	//! Terrain size and offset, read off ourselves once the world is absorbed.
	protected int m_iSizeX;
	protected int m_iSizeY;

	//------------------------------------------------------------------------
	static MCF_Map_BoardEntity Spawn(notnull IEntity near)
	{
		BaseWorld world = near.GetWorld();
		if (!world)
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		near.GetTransform(params.Transform);

		return MCF_Map_BoardEntity.Cast(GetGame().SpawnEntity(MCF_Map_BoardEntity, world, params));
	}

	//------------------------------------------------------------------------
	//! Everything SCR_MapEntity does on its way to a drawn map, minus the
	//! bookkeeping: absorb the world, build the layers from the same configs,
	//! and pick the layer the real map picked.
	void Setup(SCR_MapLayersBase layers, SCR_MapPropsBase props, int layerIndex)
	{
		// The map's precalculated form. The real map entity does this at its
		// own init; a second one has to ask for it.
		AbsorbData();

		vector size = Size();
		m_iSizeX = size[0];
		m_iSizeY = size[2];

		if (!layers || layers.m_aLayers.IsEmpty())
		{
			MCF_Core_Log.Warn("map board entity: no layer configuration, so there is nothing to draw");
			return;
		}

		int count = layers.m_aLayers.Count();
		InitializeLayers(count);

		for (int i = 0; i < count; i++)
		{
			MapLayer layer = GetLayer(i);
			if (!layer)
				continue;

			layers.m_aLayers[i].SetLayerProps(layer);

			if (!props)
				continue;

			foreach (SCR_MapPropsConfig propsCfg : props.m_aMapPropConfigs)
			{
				propsCfg.SetDefaults(layer);
			}
		}

		if (layerIndex >= 0 && layerIndex < count)
			SetLayer(layerIndex);

		MCF_Core_Log.Debug("map board entity ready: " + m_iSizeX.ToString() + " x " + m_iSizeY.ToString() + " m, " + count.ToString() + " layer(s)");
	}

	//------------------------------------------------------------------------
	//! The board's view, in the four things a map entity keeps. Cheap enough
	//! to re-state on a tick, and re-stating is what makes it stick.
	void ShowView(float zoomLevel, vector pan, vector frameMin, vector frameMax, bool grid)
	{
		EnableVisualisation(true);
		EnableGrid(grid);

		if (zoomLevel > 0)
			ZoomChange(zoomLevel);

		PosChange(pan[0], pan[1]);
		SetFrame(frameMin, frameMax);
	}

	//------------------------------------------------------------------------
	void Hide()
	{
		EnableVisualisation(false);
	}
}
