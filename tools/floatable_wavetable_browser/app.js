const TARGET_SLOT_COUNT = 16;
const CYCLE_POINT_COUNT = 256;
const RESULT_RENDER_LIMIT = 160;
const DEFAULT_PREVIEW_FREQUENCY_HZ = 110;
const DEFAULT_MORPH_DURATION_SECONDS = 10;
const DRAG_PAYLOAD_MIME = "application/x-mtws-curator-drag";
const FILTER_SLIDER_CONFIGS = [
  {
    groupKey: "zeroCrossings",
    minStateKey: "zeroCrossingsMin",
    maxStateKey: "zeroCrossingsMax",
    minInputId: "zeroCrossMinInput",
    maxInputId: "zeroCrossMaxInput",
    fillId: "zeroCrossRangeFill",
    readoutId: "zeroCrossReadout",
    format: "integer",
    defaultBounds: { min: 0, max: 0, step: 1 },
  },
  {
    groupKey: "brightness",
    minStateKey: "brightnessMinPercent",
    maxStateKey: "brightnessMaxPercent",
    minInputId: "brightnessMinInput",
    maxInputId: "brightnessMaxInput",
    fillId: "brightnessRangeFill",
    readoutId: "brightnessReadout",
    format: "percent",
    defaultBounds: { min: 0, max: 100, step: 1 },
  },
  {
    groupKey: "harmonic",
    minStateKey: "harmonicMin",
    maxStateKey: "harmonicMax",
    minInputId: "harmonicMinInput",
    maxInputId: "harmonicMaxInput",
    fillId: "harmonicRangeFill",
    readoutId: "harmonicReadout",
    format: "integer",
    defaultBounds: { min: 1, max: 16, step: 1 },
  },
  {
    groupKey: "high",
    minStateKey: "highMinPercent",
    maxStateKey: "highMaxPercent",
    minInputId: "highMinInput",
    maxInputId: "highMaxInput",
    fillId: "highRangeFill",
    readoutId: "highReadout",
    format: "percent",
    defaultBounds: { min: 0, max: 100, step: 1 },
  },
  {
    groupKey: "odd",
    minStateKey: "oddMinPercent",
    maxStateKey: "oddMaxPercent",
    minInputId: "oddMinInput",
    maxInputId: "oddMaxInput",
    fillId: "oddRangeFill",
    readoutId: "oddReadout",
    format: "percent",
    defaultBounds: { min: 0, max: 100, step: 1 },
  },
];
const FILTER_SLIDER_CONFIG_MAP = new Map(
  FILTER_SLIDER_CONFIGS.map((config) => [config.groupKey, config]),
);

const state = {
  manifest: null,
  entryMap: new Map(),
  filteredEntries: [],
  selectedEntryId: null,
  selectedSlotIndex: 0,
  slots: Array.from({ length: TARGET_SLOT_COUNT }, () => null),
  categoryFilters: [],
  libraryFilters: {
    zeroCrossingsMin: 0,
    zeroCrossingsMax: 0,
    brightnessMinPercent: 0,
    brightnessMaxPercent: 100,
    harmonicMin: 1,
    harmonicMax: 16,
    highMinPercent: 0,
    highMaxPercent: 100,
    oddMinPercent: 0,
    oddMaxPercent: 100,
  },
  filterBounds: {
    zeroCrossings: { min: 0, max: 0, step: 1 },
    brightness: { min: 0, max: 100, step: 1 },
    harmonic: { min: 1, max: 16, step: 1 },
    high: { min: 0, max: 100, step: 1 },
    odd: { min: 0, max: 100, step: 1 },
  },
  sortKey: "name",
  audioContext: null,
  activeSource: null,
  activeGain: null,
  activeDragPayload: null,
  waveCache: new Map(),
  waveLoadPromises: new Map(),
};

const dom = {
  assignNextSlotButton: null,
  assignSelectedSlotButton: null,
  categoryFilterDetails: null,
  categoryList: null,
  categorySummary: null,
  candidateCanvas: null,
  candidateMeta: null,
  candidateTitle: null,
  exportJsonButton: null,
  exportWavButton: null,
  filterSliderGroups: new Map(),
  filterSliderInputs: [],
  libraryResults: null,
  manifestSummary: null,
  morphDurationInput: null,
  playMorphButtons: [],
  previewCandidateButton: null,
  previewPitchInput: null,
  previewTargetSlotButton: null,
  randomAllButton: null,
  randomTargetButton: null,
  resetCategoryFiltersButton: null,
  resultsSummary: null,
  selectedSlotSummary: null,
  slotGrid: null,
  sortSelect: null,
  statusLine: null,
  stopPlaybackButton: null,
};

/**
 * Bootstraps the curator once the DOM is ready.
 * Inputs: none.
 * Outputs: none. The page is bound, rendered, and starts loading the AKWF manifest.
 */
function initializeApp() {
  cacheDomReferences();
  bindControlEvents();
  renderAll();
  void loadManifest();
}

/**
 * Cache all DOM elements used by the interface.
 * Inputs: none.
 * Outputs: none. The dom lookup object is populated with element references.
 */
function cacheDomReferences() {
  dom.assignNextSlotButton = document.getElementById("assignNextSlotButton");
  dom.assignSelectedSlotButton = document.getElementById("assignSelectedSlotButton");
  dom.categoryFilterDetails = document.getElementById("categoryFilterDetails");
  dom.categoryList = document.getElementById("categoryList");
  dom.categorySummary = document.getElementById("categorySummary");
  dom.candidateCanvas = document.getElementById("candidateCanvas");
  dom.candidateMeta = document.getElementById("candidateMeta");
  dom.candidateTitle = document.getElementById("candidateTitle");
  dom.exportJsonButton = document.getElementById("exportJsonButton");
  dom.exportWavButton = document.getElementById("exportWavButton");
  dom.filterSliderInputs = Array.from(document.querySelectorAll("[data-slider-group]"));
  dom.filterSliderGroups = new Map();
  FILTER_SLIDER_CONFIGS.forEach((config) => {
    dom.filterSliderGroups.set(config.groupKey, {
      minInput: document.getElementById(config.minInputId),
      maxInput: document.getElementById(config.maxInputId),
      fill: document.getElementById(config.fillId),
      readout: document.getElementById(config.readoutId),
    });
  });
  dom.libraryResults = document.getElementById("libraryResults");
  dom.manifestSummary = document.getElementById("manifestSummary");
  dom.morphDurationInput = document.getElementById("morphDurationInput");
  dom.playMorphButtons = Array.from(document.querySelectorAll("[data-morph-trigger]"));
  dom.previewCandidateButton = document.getElementById("previewCandidateButton");
  dom.previewPitchInput = document.getElementById("previewPitchInput");
  dom.previewTargetSlotButton = document.getElementById("previewTargetSlotButton");
  dom.randomAllButton = document.getElementById("randomAllButton");
  dom.randomTargetButton = document.getElementById("randomTargetButton");
  dom.resetCategoryFiltersButton = document.getElementById("resetCategoryFiltersButton");
  dom.resultsSummary = document.getElementById("resultsSummary");
  dom.selectedSlotSummary = document.getElementById("selectedSlotSummary");
  dom.slotGrid = document.getElementById("slotGrid");
  dom.sortSelect = document.getElementById("sortSelect");
  dom.statusLine = document.getElementById("statusLine");
  dom.stopPlaybackButton = document.getElementById("stopPlaybackButton");
}

/**
 * Attach all control handlers used by the library, slot board, and export panel.
 * Inputs: none.
 * Outputs: none. Browser events are connected to the state update functions.
 */
function bindControlEvents() {
  dom.filterSliderInputs.forEach((input) => {
    input.addEventListener("input", handleLibraryFilterSliderInput);
  });

  dom.sortSelect.addEventListener("change", (event) => {
    state.sortKey = String(event.target.value || "name");
    applyFiltersAndRender();
  });

  dom.resetCategoryFiltersButton.addEventListener("click", () => {
    resetCategoryFilters();
  });

  dom.previewCandidateButton.addEventListener("click", () => {
    void playSelectedCandidate();
  });

  dom.assignSelectedSlotButton.addEventListener("click", () => {
    assignSelectedEntryToSelectedSlot();
  });

  dom.assignNextSlotButton.addEventListener("click", () => {
    assignSelectedEntryToNextOpenSlot();
  });

  dom.previewTargetSlotButton.addEventListener("click", () => {
    void playSelectedSlot();
  });

  dom.playMorphButtons.forEach((button) => {
    button.addEventListener("click", () => {
      void playMorphSweep();
    });
  });

  dom.randomTargetButton.addEventListener("click", () => {
    assignRandomFilteredEntryToSelectedSlot();
  });

  dom.randomAllButton.addEventListener("click", () => {
    fillSlotsFromFilteredEntries();
  });

  dom.stopPlaybackButton.addEventListener("click", () => {
    stopPlayback();
  });

  dom.exportJsonButton.addEventListener("click", () => {
    void exportSelectionJson();
  });

  dom.exportWavButton.addEventListener("click", () => {
    void exportBankWav();
  });

  window.addEventListener("beforeunload", () => {
    stopPlayback();
  });

  window.addEventListener("resize", () => {
    renderCandidatePanel();
    renderSlotGrid();
    renderLibraryResults();
  });
}

/**
 * Load the generated AKWF manifest from the current tool folder.
 * Inputs: none.
 * Outputs: Promise<void>. The manifest is parsed, filtered, and rendered into the UI.
 */
async function loadManifest() {
  setStatusMessage("Loading AKWF manifest...");

  try {
    const response = await fetch("./akwf_manifest.json");
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    state.manifest = await response.json();
    buildEntryMap();
    initializeLibraryFilterBoundsFromManifest();
    populateCategoryFilterList();
    applyFiltersAndRender();
    applyDemoSelectionIfRequested();

    const categoryCount = Array.isArray(state.manifest.categories) ? state.manifest.categories.length : 0;
    setStatusMessage(
      `Loaded ${formatInteger(state.manifest.wave_count || 0)} waves across ${formatInteger(categoryCount)} categories.`,
    );
  } catch (error) {
    state.manifest = null;
    state.entryMap = new Map();
    state.filteredEntries = [];
    setStatusMessage(`Could not load manifest: ${String(error.message || error)}`);
    renderAll();
  }
}

/**
 * Build a fast ID lookup map from the loaded manifest entries.
 * Inputs: none.
 * Outputs: none. state.entryMap is repopulated from manifest wave entries.
 */
function buildEntryMap() {
  const entryMap = new Map();
  const manifestWaves = Array.isArray(state.manifest?.waves) ? state.manifest.waves : [];

  manifestWaves.forEach((entry) => {
    entryMap.set(entry.id, entry);
  });

  state.entryMap = entryMap;
}

/**
 * Populate the category filter checklist using the counts stored in the manifest.
 * Inputs: none.
 * Outputs: none. The category filter list is rebuilt from manifest category metadata.
 */
function populateCategoryFilterList() {
  dom.categoryList.textContent = "";
  const categories = Array.isArray(state.manifest?.categories) ? state.manifest.categories : [];
  const validCategoryNames = new Set(categories.map((category) => category.name));
  state.categoryFilters = state.categoryFilters.filter((categoryName) => validCategoryNames.has(categoryName));

  categories.forEach((category) => {
    const label = document.createElement("label");
    label.className = "category-option";

    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.value = category.name;
    checkbox.checked = state.categoryFilters.includes(category.name);
    checkbox.addEventListener("change", () => {
      updateCategoryFilter(category.name, checkbox.checked);
    });

    const text = document.createElement("span");
    text.textContent = `${category.name} (${formatInteger(category.count)})`;

    label.appendChild(checkbox);
    label.appendChild(text);
    dom.categoryList.appendChild(label);
  });

  updateCategorySummary();
}

/**
 * Update one category filter entry in response to a checkbox toggle.
 * Inputs: categoryName - category string tied to one checkbox, isEnabled - true when the category is selected.
 * Outputs: none. The category filter state updates and the library is refiltered.
 */
function updateCategoryFilter(categoryName, isEnabled) {
  const categoryFilters = new Set(state.categoryFilters);
  if (isEnabled) {
    categoryFilters.add(categoryName);
  } else {
    categoryFilters.delete(categoryName);
  }

  state.categoryFilters = Array.from(categoryFilters);
  updateCategorySummary();
  applyFiltersAndRender();
}

/**
 * Reset the category filter back to the default "all categories" state.
 * Inputs: none.
 * Outputs: none. All category checkboxes are cleared and the library is refiltered.
 */
function resetCategoryFilters() {
  state.categoryFilters = [];
  dom.categoryList.querySelectorAll('input[type="checkbox"]').forEach((checkbox) => {
    checkbox.checked = false;
  });
  updateCategorySummary();
  applyFiltersAndRender();
}

/**
 * Initialize all slider bounds from the loaded manifest and reset the live filter values to match them.
 * Inputs: none.
 * Outputs: none. Slider bounds and the current library filter state are updated from manifest metadata.
 */
function initializeLibraryFilterBoundsFromManifest() {
  const manifestWaves = Array.isArray(state.manifest?.waves) ? state.manifest.waves : [];
  const zeroCrossingValues = manifestWaves.map((entry) => Number(entry.zero_crossings || 0));
  const zeroCrossingBounds = buildSliderBoundsFromValues(zeroCrossingValues, 0, 0, 1);

  state.filterBounds = {
    zeroCrossings: zeroCrossingBounds,
    brightness: { min: 0, max: 100, step: 1 },
    harmonic: { min: 1, max: 16, step: 1 },
    high: { min: 0, max: 100, step: 1 },
    odd: { min: 0, max: 100, step: 1 },
  };

  FILTER_SLIDER_CONFIGS.forEach((config) => {
    const bounds = state.filterBounds[config.groupKey] || config.defaultBounds;
    state.libraryFilters[config.minStateKey] = bounds.min;
    state.libraryFilters[config.maxStateKey] = bounds.max;
  });

  syncAllLibraryFilterSliders();
}

/**
 * Build a slider bound object from one metric list while keeping the configured step size.
 * Inputs: values - metric values from the manifest, fallbackMin/fallbackMax - safe values used when the list is empty,
 * step - slider increment to expose in the UI.
 * Outputs: object containing integer min, max, and step values for one slider group.
 */
function buildSliderBoundsFromValues(values, fallbackMin, fallbackMax, step) {
  if (values.length === 0) {
    return { min: fallbackMin, max: fallbackMax, step };
  }

  const minValue = Math.floor(Math.min(...values));
  const maxValue = Math.ceil(Math.max(...values));
  return {
    min: Number.isFinite(minValue) ? minValue : fallbackMin,
    max: Number.isFinite(maxValue) ? Math.max(minValue, maxValue) : fallbackMax,
    step,
  };
}

/**
 * Apply one slider edit from the compact dual-thumb filter controls.
 * Inputs: event - browser input event fired from one of the slider thumbs.
 * Outputs: none. The targeted filter bounds update, the slider redraws, and the library is refiltered.
 */
function handleLibraryFilterSliderInput(event) {
  const input = event.target;
  const groupKey = String(input.dataset.sliderGroup || "");
  const rangeRole = String(input.dataset.rangeRole || "");
  const config = FILTER_SLIDER_CONFIG_MAP.get(groupKey);
  const bounds = state.filterBounds[groupKey];

  if (!config || !bounds || (rangeRole !== "min" && rangeRole !== "max")) {
    return;
  }

  const numericValue = clampNumericValue(Number(input.value), bounds.min, bounds.max);
  if (rangeRole === "min") {
    state.libraryFilters[config.minStateKey] = Math.min(
      numericValue,
      state.libraryFilters[config.maxStateKey],
    );
  } else {
    state.libraryFilters[config.maxStateKey] = Math.max(
      numericValue,
      state.libraryFilters[config.minStateKey],
    );
  }

  syncLibraryFilterSlider(groupKey);
  applyFiltersAndRender();
}

/**
 * Clamp one numeric value into a valid slider range.
 * Inputs: value - raw slider number, minValue - inclusive lower bound, maxValue - inclusive upper bound.
 * Outputs: bounded numeric value that stays inside the slider range.
 */
function clampNumericValue(value, minValue, maxValue) {
  if (!Number.isFinite(value)) {
    return minValue;
  }

  return Math.min(maxValue, Math.max(minValue, value));
}

/**
 * Redraw every dual-range slider so the thumbs, fill bar, and readout match application state.
 * Inputs: none.
 * Outputs: none. Each metric slider element is synchronized from state.filterBounds and state.libraryFilters.
 */
function syncAllLibraryFilterSliders() {
  FILTER_SLIDER_CONFIGS.forEach((config) => {
    syncLibraryFilterSlider(config.groupKey);
  });
}

/**
 * Redraw one dual-range slider group from the current filter bounds and values.
 * Inputs: groupKey - stable slider group name to synchronize.
 * Outputs: none. The group's two thumbs, fill bar, and text readout are updated.
 */
function syncLibraryFilterSlider(groupKey) {
  const config = FILTER_SLIDER_CONFIG_MAP.get(groupKey);
  const sliderGroup = dom.filterSliderGroups.get(groupKey);
  const bounds = state.filterBounds[groupKey] || config?.defaultBounds;

  if (!config || !sliderGroup || !bounds) {
    return;
  }

  const minValue = clampNumericValue(state.libraryFilters[config.minStateKey], bounds.min, bounds.max);
  const maxValue = clampNumericValue(state.libraryFilters[config.maxStateKey], bounds.min, bounds.max);
  state.libraryFilters[config.minStateKey] = Math.min(minValue, maxValue);
  state.libraryFilters[config.maxStateKey] = Math.max(maxValue, minValue);

  [sliderGroup.minInput, sliderGroup.maxInput].forEach((input) => {
    input.min = String(bounds.min);
    input.max = String(bounds.max);
    input.step = String(bounds.step);
  });

  sliderGroup.minInput.value = String(state.libraryFilters[config.minStateKey]);
  sliderGroup.maxInput.value = String(state.libraryFilters[config.maxStateKey]);

  if (sliderGroup.readout) {
    sliderGroup.readout.textContent = formatSliderRangeReadout(
      config.format,
      state.libraryFilters[config.minStateKey],
      state.libraryFilters[config.maxStateKey],
    );
  }

  if (sliderGroup.fill) {
    const rangeSpan = Math.max(1, bounds.max - bounds.min);
    const leftPercent = ((state.libraryFilters[config.minStateKey] - bounds.min) / rangeSpan) * 100;
    const rightPercent = ((state.libraryFilters[config.maxStateKey] - bounds.min) / rangeSpan) * 100;
    sliderGroup.fill.style.left = `${leftPercent}%`;
    sliderGroup.fill.style.width = `${Math.max(0, rightPercent - leftPercent)}%`;
  }
}

/**
 * Format one slider readout using either integer or percent display rules.
 * Inputs: format - readout mode string, minValue/maxValue - inclusive bounds currently selected in the slider.
 * Outputs: compact human-readable readout text shown next to the slider label.
 */
function formatSliderRangeReadout(format, minValue, maxValue) {
  if (format === "percent") {
    return `${formatInteger(minValue)}-${formatInteger(maxValue)}%`;
  }

  return `${formatInteger(minValue)}-${formatInteger(maxValue)}`;
}

/**
 * Render a compact summary of the active category filter selection.
 * Inputs: none.
 * Outputs: none. The disclosure header text reflects the current category selection count.
 */
function updateCategorySummary() {
  if (!dom.categorySummary) {
    return;
  }

  if (state.categoryFilters.length === 0) {
    dom.categorySummary.textContent = "All categories";
  } else if (state.categoryFilters.length === 1) {
    dom.categorySummary.textContent = state.categoryFilters[0];
  } else {
    dom.categorySummary.textContent = `${formatInteger(state.categoryFilters.length)} selected`;
  }

  if (dom.resetCategoryFiltersButton) {
    dom.resetCategoryFiltersButton.disabled = state.categoryFilters.length === 0;
  }
}

/**
 * Test whether one numeric value falls inside an optional inclusive min/max range.
 * Inputs: value - numeric metric to test, minValue - inclusive lower bound or null,
 * maxValue - inclusive upper bound or null.
 * Outputs: true when the value satisfies the present bounds, otherwise false.
 */
function valueMatchesNumericRange(value, minValue, maxValue) {
  if (minValue !== null && value < minValue) {
    return false;
  }

  if (maxValue !== null && value > maxValue) {
    return false;
  }

  return true;
}

/**
 * Recompute the filtered library result set and redraw the interface.
 * Inputs: none.
 * Outputs: none. state.filteredEntries is refreshed and the full UI rerenders.
 */
function applyFiltersAndRender() {
  state.filteredEntries = filterEntries();

  if (!state.selectedEntryId || !state.entryMap.has(state.selectedEntryId)) {
    state.selectedEntryId = state.filteredEntries[0]?.id || null;
  }

  if (state.selectedEntryId && !state.filteredEntries.some((entry) => entry.id === state.selectedEntryId)) {
    state.selectedEntryId = state.filteredEntries[0]?.id || null;
  }

  renderAll();
}

/**
 * Filter and sort the full manifest using the current category and metric settings.
 * Inputs: none.
 * Outputs: array of manifest entries in the current library order.
 */
function filterEntries() {
  const manifestWaves = Array.isArray(state.manifest?.waves) ? state.manifest.waves : [];
  const {
    zeroCrossingsMin,
    zeroCrossingsMax,
    brightnessMinPercent,
    brightnessMaxPercent,
    harmonicMin,
    harmonicMax,
    highMinPercent,
    highMaxPercent,
    oddMinPercent,
    oddMaxPercent,
  } = state.libraryFilters;

  const filtered = manifestWaves.filter((entry) => {
    if (state.categoryFilters.length > 0 && !state.categoryFilters.includes(entry.category)) {
      return false;
    }

    if (!valueMatchesNumericRange(entry.zero_crossings, zeroCrossingsMin, zeroCrossingsMax)) {
      return false;
    }

    if (!valueMatchesNumericRange(entry.brightness_score * 100, brightnessMinPercent, brightnessMaxPercent)) {
      return false;
    }

    if (!valueMatchesNumericRange(entry.dominant_harmonic, harmonicMin, harmonicMax)) {
      return false;
    }

    if (!valueMatchesNumericRange(entry.high_harmonic_ratio * 100, highMinPercent, highMaxPercent)) {
      return false;
    }

    if (!valueMatchesNumericRange(entry.odd_harmonic_ratio * 100, oddMinPercent, oddMaxPercent)) {
      return false;
    }

    return true;
  });

  return sortEntries(filtered.slice(), state.sortKey);
}

/**
 * Sort a result list according to one of the user-visible sort modes.
 * Inputs: entries - filtered manifest entries, sortKey - selected sort mode string.
 * Outputs: sorted array reference containing the same entries in new order.
 */
function sortEntries(entries, sortKey) {
  const compareText = (left, right) => {
    const categoryCompare = left.category.localeCompare(right.category);
    if (categoryCompare !== 0) {
      return categoryCompare;
    }
    return left.display_name.localeCompare(right.display_name);
  };

  entries.sort((left, right) => {
    if (sortKey === "brightness_desc") {
      return right.brightness_score - left.brightness_score || compareText(left, right);
    }

    if (sortKey === "zero_crossings_asc") {
      return left.zero_crossings - right.zero_crossings || compareText(left, right);
    }

    if (sortKey === "zero_crossings_desc") {
      return right.zero_crossings - left.zero_crossings || compareText(left, right);
    }

    if (sortKey === "odd_ratio_desc") {
      return right.odd_harmonic_ratio - left.odd_harmonic_ratio || compareText(left, right);
    }

    if (sortKey === "dominant_harmonic_asc") {
      return left.dominant_harmonic - right.dominant_harmonic || compareText(left, right);
    }

    return compareText(left, right);
  });

  return entries;
}

/**
 * Redraw the manifest summary, library results, candidate panel, slot grid, and button state.
 * Inputs: options - optional render flags. preserveLibraryScroll keeps the current library scroll offset
 * when the library contents are being redrawn without changing the filtered result set.
 * Outputs: none. The DOM is updated to match the current application state.
 */
function renderAll(options = {}) {
  syncAllLibraryFilterSliders();
  updateCategorySummary();
  renderManifestSummary();
  renderResultsSummary();
  renderLibraryResults({
    preserveScroll: Boolean(options.preserveLibraryScroll),
  });
  renderCandidatePanel();
  renderSlotGrid();
  updateActionButtons();
}

/**
 * Render the top-level manifest summary block.
 * Inputs: none.
 * Outputs: none. The manifest summary line is written into the summary panel.
 */
function renderManifestSummary() {
  if (!dom.manifestSummary) {
    return;
  }

  if (!state.manifest) {
    dom.manifestSummary.textContent = "Manifest not loaded. Generate it first, then serve this folder.";
    return;
  }

  const categoryCount = Array.isArray(state.manifest.categories) ? state.manifest.categories.length : 0;
  dom.manifestSummary.textContent =
    `${formatInteger(state.manifest.wave_count || 0)} waves, ${formatInteger(categoryCount)} categories, ` +
    `${formatInteger(state.manifest.analysis_point_count || CYCLE_POINT_COUNT)}-point analysis, ` +
    `${formatInteger(state.manifest.preview_point_count || 64)}-point previews.`;
}

/**
 * Render the line that summarizes the current filtered library result set.
 * Inputs: none.
 * Outputs: none. The library header shows how many results are currently visible.
 */
function renderResultsSummary() {
  if (!dom.resultsSummary) {
    return;
  }

  if (!state.manifest) {
    dom.resultsSummary.textContent = "Loading library...";
    return;
  }

  const filteredCount = state.filteredEntries.length;
  const visibleCount = Math.min(filteredCount, RESULT_RENDER_LIMIT);
  const hiddenCount = filteredCount - visibleCount;

  dom.resultsSummary.textContent = hiddenCount > 0
    ? `${formatInteger(visibleCount)} / ${formatInteger(filteredCount)} shown`
    : `${formatInteger(filteredCount)} shown`;
}

/**
 * Render the scrollable library result cards using the current filtered entries.
 * Inputs: options - optional render flags. preserveScroll restores the current scroll offset after redraw.
 * Outputs: none. The library result container is rebuilt from the filtered entry list.
 */
function renderLibraryResults(options = {}) {
  const previousScrollTop = options.preserveScroll ? dom.libraryResults.scrollTop : 0;
  dom.libraryResults.textContent = "";

  if (!state.manifest) {
    dom.libraryResults.appendChild(
      createPlaceholderCard("Manifest not available. Generate akwf_manifest.json first."),
    );
    if (options.preserveScroll) {
      dom.libraryResults.scrollTop = previousScrollTop;
    }
    return;
  }

  if (state.filteredEntries.length === 0) {
    dom.libraryResults.appendChild(
      createPlaceholderCard("No waves match the current filter."),
    );
    if (options.preserveScroll) {
      dom.libraryResults.scrollTop = previousScrollTop;
    }
    return;
  }

  const visibleEntries = state.filteredEntries.slice(0, RESULT_RENDER_LIMIT);
  visibleEntries.forEach((entry) => {
    const card = createLibraryCard(entry);
    dom.libraryResults.appendChild(card);
    drawWaveCanvas(card.querySelector(".mini-canvas"), getEntryPreviewPoints(entry), "#0f6b6a");
  });

  if (options.preserveScroll) {
    dom.libraryResults.scrollTop = previousScrollTop;
  }
}

/**
 * Build one library card showing preview shape, metadata chips, and assignment controls.
 * Inputs: entry - one manifest entry from the filtered result set.
 * Outputs: HTMLElement representing one result card in the scrollable library panel.
 */
function createLibraryCard(entry) {
  const card = document.createElement("article");
  card.className = "library-card";
  card.draggable = true;
  if (entry.id === state.selectedEntryId) {
    card.classList.add("is-selected");
  }

  card.addEventListener("click", () => {
    selectEntry(entry.id);
  });

  card.addEventListener("dragstart", (event) => {
    startInternalDrag(event.dataTransfer, "library-entry", entry.id, "copy");
  });

  card.addEventListener("dragend", () => {
    endInternalDrag();
  });

  const header = document.createElement("div");
  header.className = "library-card-header";

  const textColumn = document.createElement("div");
  const title = document.createElement("p");
  title.className = "card-title";
  title.textContent = entry.display_name;

  const subtitle = document.createElement("p");
  subtitle.className = "card-subtitle";
  subtitle.textContent = entry.category;

  textColumn.appendChild(title);
  textColumn.appendChild(subtitle);

  const emphasis = document.createElement("span");
  emphasis.className = "metric-pill";
  emphasis.textContent = `H${entry.dominant_harmonic}`;

  header.appendChild(textColumn);
  header.appendChild(emphasis);

  const previewCanvas = document.createElement("canvas");
  previewCanvas.className = "mini-canvas";

  const chipRow = document.createElement("div");
  chipRow.className = "chip-row";

  chipRow.appendChild(createChip(`zc ${formatInteger(entry.zero_crossings)}`));
  chipRow.appendChild(createChip(`bright ${formatPercent(entry.brightness_score)}`));
  chipRow.appendChild(createChip(`odd ${formatPercent(entry.odd_harmonic_ratio)}`));

  const actions = document.createElement("div");
  actions.className = "card-actions";

  const inspectButton = document.createElement("button");
  inspectButton.className = "ghost-button";
  inspectButton.type = "button";
  inspectButton.textContent = "Inspect";
  inspectButton.addEventListener("click", (event) => {
    event.stopPropagation();
    selectEntry(entry.id);
  });

  const placeButton = document.createElement("button");
  placeButton.className = "ghost-button";
  placeButton.type = "button";
  placeButton.textContent = `Add ${String(state.selectedSlotIndex + 1).padStart(2, "0")}`;
  placeButton.addEventListener("click", (event) => {
    event.stopPropagation();
    selectEntry(entry.id);
    assignEntryToSlot(entry.id, state.selectedSlotIndex);
  });

  actions.appendChild(inspectButton);
  actions.appendChild(placeButton);

  card.appendChild(header);
  card.appendChild(previewCanvas);
  card.appendChild(chipRow);
  card.appendChild(actions);
  return card;
}

/**
 * Render the currently selected candidate entry and its metadata panel.
 * Inputs: none.
 * Outputs: none. The candidate panel is redrawn from the selected manifest entry.
 */
function renderCandidatePanel() {
  const entry = getSelectedEntry();

  if (!entry) {
    dom.candidateTitle.textContent = "No Wave Selected";
    drawEmptyCanvasState(dom.candidateCanvas, "Choose a library wave");
    dom.candidateMeta.innerHTML = '<p class="placeholder-copy">Select a wave.</p>';
    dom.selectedSlotSummary.textContent = `Slot ${String(state.selectedSlotIndex + 1).padStart(2, "0")}`;
    return;
  }

  const cachedWave = state.waveCache.get(entry.id);
  const drawPoints = cachedWave ? cachedWave.points256 : getEntryPreviewPoints(entry);

  dom.candidateTitle.textContent = entry.display_name;
  drawWaveCanvas(dom.candidateCanvas, drawPoints, "#c86a18");
  dom.candidateMeta.innerHTML = buildCandidateMetaMarkup(entry);

  const targetSlot = state.slots[state.selectedSlotIndex];
  const targetSuffix = targetSlot
    ? `replace ${getEntryById(targetSlot.entryId)?.display_name || "current wave"}`
    : "empty";
  dom.selectedSlotSummary.textContent =
    `Slot ${String(state.selectedSlotIndex + 1).padStart(2, "0")} / ${targetSuffix}`;

  void warmEntryCycle(entry.id);
}

/**
 * Build the HTML used inside the candidate metadata panel.
 * Inputs: entry - selected manifest entry.
 * Outputs: string containing metadata cards and summary chips for the candidate panel.
 */
function buildCandidateMetaMarkup(entry) {
  const metaCards = [
    createMetaCardMarkup("Category", entry.category),
    createMetaCardMarkup("Sample Count", formatInteger(entry.sample_count)),
    createMetaCardMarkup("Zero Crossings", formatInteger(entry.zero_crossings)),
    createMetaCardMarkup("Dominant Harmonic", `H${formatInteger(entry.dominant_harmonic)}`),
    createMetaCardMarkup("Brightness", formatPercent(entry.brightness_score)),
    createMetaCardMarkup("High Harmonics", formatPercent(entry.high_harmonic_ratio)),
    createMetaCardMarkup("Odd Harmonics", formatPercent(entry.odd_harmonic_ratio)),
    createMetaCardMarkup("Spectral Centroid", formatDecimal(entry.spectral_centroid)),
  ];

  const chips = [
    `dc ${formatSignedDecimal(entry.dc_offset)}`,
    `peak ${formatDecimal(entry.peak_abs)}`,
    `rms ${formatDecimal(entry.rms)}`,
  ]
    .map((chipText) => `<span class="chip">${chipText}</span>`)
    .join("");

  return `${metaCards.join("")}<div class="chip-row">${chips}</div>`;
}

/**
 * Build one compact HTML fragment for a metadata card.
 * Inputs: label - card title, value - displayed metric value.
 * Outputs: string containing a metadata card for candidate details.
 */
function createMetaCardMarkup(label, value) {
  return (
    `<div class="meta-card">` +
    `<span class="meta-label">${label}</span>` +
    `<span class="meta-value">${value}</span>` +
    `</div>`
  );
}

/**
 * Render the current 16-slot board including previews, reorder actions, and slot targeting.
 * Inputs: none.
 * Outputs: none. The slot grid DOM is rebuilt from the current slot state.
 */
function renderSlotGrid() {
  dom.slotGrid.textContent = "";

  state.slots.forEach((slot, slotIndex) => {
    const card = createSlotCard(slot, slotIndex);
    dom.slotGrid.appendChild(card);

    if (slot) {
      drawWaveCanvas(card.querySelector(".slot-canvas"), getEntryWavePointsForDraw(slot.entryId), "#17202d");
    }
  });
}

/**
 * Build one slot card representing either an empty position or an assigned wave.
 * Inputs: slot - current slot object or null, slotIndex - zero-based slot position.
 * Outputs: HTMLElement for the 4x4 slot board.
 */
function createSlotCard(slot, slotIndex) {
  const card = document.createElement("article");
  card.className = "slot-card";

  if (slotIndex === state.selectedSlotIndex) {
    card.classList.add("is-selected");
  }

  if (!slot) {
    card.classList.add("is-empty");
  }

  card.addEventListener("click", () => {
    setSelectedSlot(slotIndex);
  });

  card.addEventListener("dragover", (event) => {
    const payload = getActiveDragPayload(event.dataTransfer);
    if (!payload) {
      return;
    }

    event.preventDefault();
    card.classList.add("is-drop-target");
    event.dataTransfer.dropEffect = payload.kind === "library-entry" ? "copy" : "move";
  });

  card.addEventListener("dragenter", (event) => {
    if (getActiveDragPayload(event.dataTransfer)) {
      card.classList.add("is-drop-target");
    }
  });

  card.addEventListener("dragleave", (event) => {
    if (!card.contains(event.relatedTarget)) {
      card.classList.remove("is-drop-target");
    }
  });

  card.addEventListener("drop", (event) => {
    event.preventDefault();
    const payload = getActiveDragPayload(event.dataTransfer);
    endInternalDrag();
    if (!payload) {
      return;
    }

    if (payload.kind === "slot-index") {
      const fromIndex = Number(payload.value);
      if (!Number.isNaN(fromIndex) && fromIndex !== slotIndex) {
        swapSlots(fromIndex, slotIndex);
      }
      return;
    }

    if (payload.kind === "library-entry" && typeof payload.value === "string") {
      state.selectedEntryId = payload.value;
      state.selectedSlotIndex = slotIndex;
      assignEntryToSlot(payload.value, slotIndex);
    }
  });

  const header = document.createElement("div");
  header.className = "slot-card-header";

  const textColumn = document.createElement("div");
  const title = document.createElement("p");
  title.className = "slot-title";
  title.textContent = `Slot ${String(slotIndex + 1).padStart(2, "0")}`;

  const subtitle = document.createElement("p");
  subtitle.className = "slot-subtitle";

  textColumn.appendChild(title);
  textColumn.appendChild(subtitle);

  header.appendChild(textColumn);

  if (slotIndex === state.selectedSlotIndex) {
    const selectChip = document.createElement("span");
    selectChip.className = "metric-pill";
    selectChip.textContent = "target";
    header.appendChild(selectChip);
  }

  card.appendChild(header);

  if (!slot) {
    subtitle.textContent = "Empty";
    return card;
  }

  const entry = getEntryById(slot.entryId);
  subtitle.textContent = entry ? entry.display_name : "Unknown wave";

  const previewCanvas = document.createElement("canvas");
  previewCanvas.className = "slot-canvas";
  card.appendChild(previewCanvas);

  const metricRow = document.createElement("div");
  metricRow.className = "metric-row";
  metricRow.appendChild(createMetricPill(`zc ${formatInteger(entry.zero_crossings)}`));
  metricRow.appendChild(createMetricPill(`bright ${formatPercent(entry.brightness_score)}`));
  metricRow.appendChild(createMetricPill(`odd ${formatPercent(entry.odd_harmonic_ratio)}`));
  card.appendChild(metricRow);

  card.draggable = true;
  card.addEventListener("dragstart", (event) => {
    startInternalDrag(event.dataTransfer, "slot-index", slotIndex, "move");
  });

  card.addEventListener("dragend", () => {
    endInternalDrag();
  });

  const actions = document.createElement("div");
  actions.className = "slot-actions";

  const playButton = document.createElement("button");
  playButton.className = "ghost-button";
  playButton.type = "button";
  playButton.textContent = "Play";
  playButton.addEventListener("click", (event) => {
    event.stopPropagation();
    setSelectedSlot(slotIndex);
    void playSelectedSlot();
  });

  const clearButton = document.createElement("button");
  clearButton.className = "ghost-button";
  clearButton.type = "button";
  clearButton.textContent = "Clear";
  clearButton.addEventListener("click", (event) => {
    event.stopPropagation();
    clearSlot(slotIndex);
  });

  const leftButton = document.createElement("button");
  leftButton.className = "ghost-button";
  leftButton.type = "button";
  leftButton.textContent = "<";
  leftButton.title = "Move left";
  leftButton.disabled = slotIndex === 0;
  leftButton.addEventListener("click", (event) => {
    event.stopPropagation();
    shiftSlot(slotIndex, -1);
  });

  const rightButton = document.createElement("button");
  rightButton.className = "ghost-button";
  rightButton.type = "button";
  rightButton.textContent = ">";
  rightButton.title = "Move right";
  rightButton.disabled = slotIndex === TARGET_SLOT_COUNT - 1;
  rightButton.addEventListener("click", (event) => {
    event.stopPropagation();
    shiftSlot(slotIndex, 1);
  });

  actions.appendChild(playButton);
  actions.appendChild(clearButton);
  actions.appendChild(leftButton);
  actions.appendChild(rightButton);
  card.appendChild(actions);
  return card;
}

/**
 * Enable or disable the global action buttons based on current selection and slot state.
 * Inputs: none.
 * Outputs: none. Button disabled states are updated in place.
 */
function updateActionButtons() {
  const hasSelectedEntry = Boolean(getSelectedEntry());
  const hasTargetSlot = Boolean(state.slots[state.selectedSlotIndex]);
  const hasFilteredEntries = state.filteredEntries.length > 0;
  const filledSlotCount = getFilledSlotCount();

  dom.previewCandidateButton.disabled = !hasSelectedEntry;
  dom.assignSelectedSlotButton.disabled = !hasSelectedEntry;
  dom.assignNextSlotButton.disabled = !hasSelectedEntry;
  dom.previewTargetSlotButton.disabled = !hasTargetSlot;
  dom.playMorphButtons.forEach((button) => {
    button.disabled = filledSlotCount < 2;
  });
  dom.randomAllButton.disabled = !hasFilteredEntries;
  dom.randomTargetButton.disabled = !hasFilteredEntries;
  dom.stopPlaybackButton.disabled = !state.activeSource;
  dom.exportJsonButton.disabled = filledSlotCount === 0;
  dom.exportWavButton.disabled = filledSlotCount === 0;
}

/**
 * Update the status line with one concise message for the user.
 * Inputs: message - status string describing the latest completed or blocked action.
 * Outputs: none. The summary status line text is replaced.
 */
function setStatusMessage(message) {
  if (dom.statusLine) {
    dom.statusLine.textContent = message;
  }
}

/**
 * Return the currently selected manifest entry or null when none is selected.
 * Inputs: none.
 * Outputs: manifest entry object for state.selectedEntryId, or null.
 */
function getSelectedEntry() {
  if (!state.selectedEntryId) {
    return null;
  }
  return getEntryById(state.selectedEntryId);
}

/**
 * Look up a manifest entry by its stable ID.
 * Inputs: entryId - manifest identifier string for one AKWF wave.
 * Outputs: manifest entry object or null when the ID is not present.
 */
function getEntryById(entryId) {
  return state.entryMap.get(entryId) || null;
}

/**
 * Select a library entry and redraw the interface around that candidate.
 * Inputs: entryId - manifest ID for the chosen candidate.
 * Outputs: none. state.selectedEntryId is updated and the UI rerenders.
 */
function selectEntry(entryId) {
  state.selectedEntryId = entryId;
  renderAll({ preserveLibraryScroll: true });
}

/**
 * Update the currently targeted slot index.
 * Inputs: slotIndex - zero-based slot position that should receive future assignments.
 * Outputs: none. state.selectedSlotIndex changes and the relevant UI is redrawn.
 */
function setSelectedSlot(slotIndex) {
  state.selectedSlotIndex = slotIndex;
  renderCandidatePanel();
  renderSlotGrid();
  renderLibraryResults({ preserveScroll: true });
  updateActionButtons();
}

/**
 * Place the selected library entry into the currently targeted slot.
 * Inputs: none.
 * Outputs: none. The slot state updates when a selected entry exists.
 */
function assignSelectedEntryToSelectedSlot() {
  const entry = getSelectedEntry();
  if (!entry) {
    setStatusMessage("Select a library wave before assigning it to a slot.");
    return;
  }

  assignEntryToSlot(entry.id, state.selectedSlotIndex);
}

/**
 * Place the selected library entry into the next available empty slot.
 * Inputs: none.
 * Outputs: none. The first empty slot receives the selected wave when one exists.
 */
function assignSelectedEntryToNextOpenSlot() {
  const entry = getSelectedEntry();
  if (!entry) {
    setStatusMessage("Select a library wave before assigning it to a slot.");
    return;
  }

  const nextOpenIndex = findNextOpenSlot();
  if (nextOpenIndex < 0) {
    setStatusMessage("All 16 slots are filled. Target a slot explicitly to replace it.");
    return;
  }

  assignEntryToSlot(entry.id, nextOpenIndex);
  setSelectedSlot(nextOpenIndex);
}

/**
 * Choose one random wave from the current filtered library and place it into the targeted slot.
 * Inputs: none.
 * Outputs: none. The targeted slot receives one random filtered entry when any are available.
 */
function assignRandomFilteredEntryToSelectedSlot() {
  if (state.filteredEntries.length === 0) {
    setStatusMessage("No filtered waves are available for random target assignment.");
    return;
  }

  const randomEntry = state.filteredEntries[Math.floor(Math.random() * state.filteredEntries.length)];
  state.selectedEntryId = randomEntry.id;
  assignEntryToSlot(randomEntry.id, state.selectedSlotIndex);
}

/**
 * Fill the 16-slot bank from the current filtered library, using a random subset when needed.
 * Inputs: none.
 * Outputs: none. The slot bank is replaced from the filtered pool and the UI rerenders.
 */
function fillSlotsFromFilteredEntries() {
  if (state.filteredEntries.length === 0) {
    setStatusMessage("No filtered waves are available for random slot population.");
    return;
  }

  const selectedEntries = state.filteredEntries.length <= TARGET_SLOT_COUNT
    ? state.filteredEntries.slice(0, TARGET_SLOT_COUNT)
    : sampleRandomEntries(state.filteredEntries, TARGET_SLOT_COUNT);

  replaceSlotsFromEntries(selectedEntries);

  if (state.filteredEntries.length <= TARGET_SLOT_COUNT) {
    setStatusMessage(
      `Placed ${formatInteger(selectedEntries.length)} filtered waves into the slot board and cleared the rest.`,
    );
    return;
  }

  setStatusMessage(
    `Filled 16 slots from a random selection of ${formatInteger(state.filteredEntries.length)} filtered waves.`,
  );
}

/**
 * Replace the current slot bank from one ordered entry list and clear any remaining slot positions.
 * Inputs: entries - ordered manifest entry array that should be written into slots from the start.
 * Outputs: none. The 16-slot bank is overwritten, the first entry is selected, and the UI rerenders.
 */
function replaceSlotsFromEntries(entries) {
  for (let slotIndex = 0; slotIndex < TARGET_SLOT_COUNT; slotIndex += 1) {
    const entry = entries[slotIndex];
    state.slots[slotIndex] = entry ? { entryId: entry.id } : null;
  }

  state.selectedSlotIndex = 0;
  state.selectedEntryId = entries[0]?.id || null;
  renderAll({ preserveLibraryScroll: true });
}

/**
 * Choose a random subset without replacement from one entry list.
 * Inputs: entries - source entry array, count - number of distinct entries to sample.
 * Outputs: new array containing up to count distinct entries in randomized order.
 */
function sampleRandomEntries(entries, count) {
  const shuffledEntries = entries.slice();

  for (let index = shuffledEntries.length - 1; index > 0; index -= 1) {
    const swapIndex = Math.floor(Math.random() * (index + 1));
    const temp = shuffledEntries[index];
    shuffledEntries[index] = shuffledEntries[swapIndex];
    shuffledEntries[swapIndex] = temp;
  }

  return shuffledEntries.slice(0, count);
}

/**
 * Write one entry ID into a slot, replacing whatever was there before.
 * Inputs: entryId - manifest ID to store, slotIndex - zero-based slot position to overwrite.
 * Outputs: none. The slot board rerenders and a short status message is shown.
 */
function assignEntryToSlot(entryId, slotIndex) {
  state.slots[slotIndex] = { entryId };
  renderAll({ preserveLibraryScroll: true });

  const entry = getEntryById(entryId);
  setStatusMessage(
    `Placed ${entry?.display_name || "wave"} into slot ${String(slotIndex + 1).padStart(2, "0")}.`,
  );
}

/**
 * Find the first empty slot in the current 16-slot selection.
 * Inputs: none.
 * Outputs: zero-based slot index for the first empty slot, or -1 when all are filled.
 */
function findNextOpenSlot() {
  return state.slots.findIndex((slot) => slot === null);
}

/**
 * Clear one slot and remove its assigned wave.
 * Inputs: slotIndex - zero-based slot position that should become empty.
 * Outputs: none. The slot state updates and the UI rerenders.
 */
function clearSlot(slotIndex) {
  state.slots[slotIndex] = null;
  renderAll({ preserveLibraryScroll: true });
  setStatusMessage(`Cleared slot ${String(slotIndex + 1).padStart(2, "0")}.`);
}

/**
 * Swap the contents of two slots.
 * Inputs: leftIndex - first slot index, rightIndex - second slot index.
 * Outputs: none. The two slot payloads exchange places and the UI rerenders.
 */
function swapSlots(leftIndex, rightIndex) {
  const temp = state.slots[leftIndex];
  state.slots[leftIndex] = state.slots[rightIndex];
  state.slots[rightIndex] = temp;

  if (state.selectedSlotIndex === leftIndex) {
    state.selectedSlotIndex = rightIndex;
  } else if (state.selectedSlotIndex === rightIndex) {
    state.selectedSlotIndex = leftIndex;
  }

  renderAll({ preserveLibraryScroll: true });
  setStatusMessage(
    `Swapped slot ${String(leftIndex + 1).padStart(2, "0")} with slot ${String(rightIndex + 1).padStart(2, "0")}.`,
  );
}

/**
 * Start one internal drag operation and mirror the payload into the browser transfer object.
 * Inputs: dataTransfer - browser drag transfer object, kind - payload category string,
 * value - payload value to serialize, effectAllowed - browser drag effect hint.
 * Outputs: none. The active drag state is stored for hover/drop logic and mirrored to dataTransfer.
 */
function startInternalDrag(dataTransfer, kind, value, effectAllowed) {
  state.activeDragPayload = { kind, value };
  writeDragPayload(dataTransfer, kind, value, effectAllowed);
}

/**
 * Finish the current drag operation and clear all transient slot drop styling.
 * Inputs: none.
 * Outputs: none. The active drag payload is discarded and slot highlight classes are removed.
 */
function endInternalDrag() {
  state.activeDragPayload = null;
  clearSlotDropTargets();
}

/**
 * Return the currently active internal drag payload, or fall back to the browser transfer object.
 * Inputs: dataTransfer - browser drag transfer object for the current drag lifecycle event.
 * Outputs: payload object with kind and value fields, or null when the drag is unrelated.
 */
function getActiveDragPayload(dataTransfer) {
  if (state.activeDragPayload) {
    return state.activeDragPayload;
  }

  return readDragPayload(dataTransfer);
}

/**
 * Write a typed drag payload so slot swaps and library-to-slot drops can coexist.
 * Inputs: dataTransfer - browser drag transfer object, kind - payload category string,
 * value - payload value to serialize, effectAllowed - browser drag effect hint.
 * Outputs: none. The payload is serialized into the drag transfer object.
 */
function writeDragPayload(dataTransfer, kind, value, effectAllowed) {
  if (!dataTransfer) {
    return;
  }

  const payload = JSON.stringify({ kind, value });
  dataTransfer.setData(DRAG_PAYLOAD_MIME, payload);
  dataTransfer.setData("text/plain", payload);
  dataTransfer.effectAllowed = effectAllowed;
}

/**
 * Read one typed drag payload from the browser transfer object.
 * Inputs: dataTransfer - browser drag transfer object that may contain curator payload data.
 * Outputs: payload object with kind and value fields, or null when the drag is unrelated.
 */
function readDragPayload(dataTransfer) {
  if (!dataTransfer) {
    return null;
  }

  const payloadText = dataTransfer.getData(DRAG_PAYLOAD_MIME) || dataTransfer.getData("text/plain");
  if (!payloadText) {
    return null;
  }

  try {
    const payload = JSON.parse(payloadText);
    if (!payload || typeof payload.kind !== "string" || !("value" in payload)) {
      return null;
    }

    return payload;
  } catch {
    return null;
  }
}

/**
 * Remove the temporary slot highlight applied while a drag operation is hovering over targets.
 * Inputs: none.
 * Outputs: none. Any current slot drop-target classes are removed from the DOM.
 */
function clearSlotDropTargets() {
  document.querySelectorAll(".slot-card.is-drop-target").forEach((card) => {
    card.classList.remove("is-drop-target");
  });
}

/**
 * Swap a slot with its immediate neighbor to the left or right.
 * Inputs: slotIndex - current slot index, direction - -1 for left or +1 for right.
 * Outputs: none. Neighboring slot contents are exchanged when the destination exists.
 */
function shiftSlot(slotIndex, direction) {
  const targetIndex = slotIndex + direction;
  if (targetIndex < 0 || targetIndex >= TARGET_SLOT_COUNT) {
    return;
  }

  swapSlots(slotIndex, targetIndex);
}

/**
 * Play the currently selected candidate wave at the configured audition pitch.
 * Inputs: none.
 * Outputs: Promise<void>. Audio playback begins when the candidate exists and loads cleanly.
 */
async function playSelectedCandidate() {
  const entry = getSelectedEntry();
  if (!entry) {
    setStatusMessage("Select a library wave before previewing it.");
    return;
  }

  const cycle = await ensureEntryCycleLoaded(entry.id);
  const audioContext = await ensureAudioContext();
  const buffer = renderToneBuffer(
    cycle.points256,
    getPreviewFrequencyHz(),
    1.4,
    audioContext.sampleRate,
  );

  playMonoBuffer(buffer, audioContext.sampleRate);
  setStatusMessage(`Previewing candidate ${entry.display_name} at ${formatInteger(getPreviewFrequencyHz())} Hz.`);
}

/**
 * Play the wave currently stored in the targeted slot.
 * Inputs: none.
 * Outputs: Promise<void>. Audio playback begins when the selected slot is filled.
 */
async function playSelectedSlot() {
  const slot = state.slots[state.selectedSlotIndex];
  if (!slot) {
    setStatusMessage("Target an occupied slot before previewing it.");
    return;
  }

  const entry = getEntryById(slot.entryId);
  const cycle = await ensureEntryCycleLoaded(slot.entryId);
  const audioContext = await ensureAudioContext();
  const buffer = renderToneBuffer(
    cycle.points256,
    getPreviewFrequencyHz(),
    1.4,
    audioContext.sampleRate,
  );

  playMonoBuffer(buffer, audioContext.sampleRate);
  setStatusMessage(`Previewing slot ${String(state.selectedSlotIndex + 1).padStart(2, "0")} (${entry.display_name}).`);
}

/**
 * Play a slow morph sweep across all currently filled slots in their present order.
 * Inputs: none.
 * Outputs: Promise<void>. Audio playback begins when at least two slots are filled.
 */
async function playMorphSweep() {
  const filledSlots = state.slots.filter((slot) => slot !== null);
  if (filledSlots.length < 2) {
    setStatusMessage("Fill at least two slots before playing a morph sweep.");
    return;
  }

  const cycles = await Promise.all(
    filledSlots.map(async (slot) => {
      const loadedWave = await ensureEntryCycleLoaded(slot.entryId);
      return loadedWave.points256;
    }),
  );

  const audioContext = await ensureAudioContext();
  const morphBuffer = renderMorphBuffer(
    cycles,
    getPreviewFrequencyHz(),
    getMorphDurationSeconds(),
    audioContext.sampleRate,
  );

  playMonoBuffer(morphBuffer, audioContext.sampleRate);
  setStatusMessage(
    `Playing a ${formatDecimal(getMorphDurationSeconds())}-second morph sweep across ${formatInteger(filledSlots.length)} filled slots.`,
  );
}

/**
 * Stop any currently playing audition source.
 * Inputs: none.
 * Outputs: none. The active buffer source is stopped and button states refresh.
 */
function stopPlayback() {
  if (state.activeSource) {
    state.activeSource.stop();
    state.activeSource.disconnect();
    state.activeSource = null;
  }

  updateActionButtons();
}

/**
 * Create or resume the shared Web Audio context used for all previews.
 * Inputs: none.
 * Outputs: Promise<AudioContext>. A running audio context ready for playback.
 */
async function ensureAudioContext() {
  if (!state.audioContext) {
    const AudioContextCtor = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextCtor) {
      throw new Error("This browser does not expose AudioContext.");
    }

    state.audioContext = new AudioContextCtor();
  }

  if (state.audioContext.state === "suspended") {
    await state.audioContext.resume();
  }

  return state.audioContext;
}

/**
 * Make sure a full 256-point cycle exists in cache for the requested entry.
 * Inputs: entryId - manifest ID for the wave that should be ready for playback/export.
 * Outputs: Promise<{ points256: Float32Array, sampleRate: number, sampleCount: number }>.
 */
async function ensureEntryCycleLoaded(entryId) {
  if (state.waveCache.has(entryId)) {
    return state.waveCache.get(entryId);
  }

  if (state.waveLoadPromises.has(entryId)) {
    return state.waveLoadPromises.get(entryId);
  }

  const entry = getEntryById(entryId);
  if (!entry) {
    throw new Error(`Unknown entry ID: ${entryId}`);
  }

  const loadPromise = loadAndPrepareWave(entry)
    .then((loadedWave) => {
      state.waveCache.set(entryId, loadedWave);
      state.waveLoadPromises.delete(entryId);
      renderCandidatePanel();
      renderSlotGrid();
      return loadedWave;
    })
    .catch((error) => {
      state.waveLoadPromises.delete(entryId);
      throw error;
    });

  state.waveLoadPromises.set(entryId, loadPromise);
  return loadPromise;
}

/**
 * Fetch one WAV file from disk, parse it, and prepare a 256-point normalized cycle.
 * Inputs: entry - manifest entry describing the source file to fetch.
 * Outputs: Promise containing cached wave data used for playback and export.
 */
async function loadAndPrepareWave(entry) {
  const response = await fetch(entry.relative_path);
  if (!response.ok) {
    throw new Error(`HTTP ${response.status} while loading ${entry.relative_path}`);
  }

  const arrayBuffer = await response.arrayBuffer();
  const parsedWave = parseWaveFile(arrayBuffer);
  return {
    points256: prepareCyclePoints(parsedWave.samples, CYCLE_POINT_COUNT),
    sampleRate: parsedWave.sampleRate,
    sampleCount: parsedWave.samples.length,
  };
}

/**
 * Parse a RIFF/WAVE file directly into mono floating-point samples.
 * Inputs: arrayBuffer - raw bytes fetched from one AKWF source file.
 * Outputs: object containing mono samples and the original file sample rate.
 */
function parseWaveFile(arrayBuffer) {
  const view = new DataView(arrayBuffer);

  if (view.byteLength < 44) {
    throw new Error("WAV file is too short.");
  }

  if (readAscii(view, 0, 4) !== "RIFF" || readAscii(view, 8, 4) !== "WAVE") {
    throw new Error("Unsupported file container. Expected RIFF/WAVE.");
  }

  let audioFormat = 0;
  let bitsPerSample = 0;
  let blockAlign = 0;
  let numChannels = 0;
  let sampleRate = 0;
  let dataOffset = -1;
  let dataSize = 0;

  let chunkOffset = 12;
  while (chunkOffset + 8 <= view.byteLength) {
    const chunkId = readAscii(view, chunkOffset, 4);
    const chunkSize = view.getUint32(chunkOffset + 4, true);
    const chunkDataOffset = chunkOffset + 8;

    if (chunkId === "fmt ") {
      audioFormat = view.getUint16(chunkDataOffset + 0, true);
      numChannels = view.getUint16(chunkDataOffset + 2, true);
      sampleRate = view.getUint32(chunkDataOffset + 4, true);
      blockAlign = view.getUint16(chunkDataOffset + 12, true);
      bitsPerSample = view.getUint16(chunkDataOffset + 14, true);
    } else if (chunkId === "data") {
      dataOffset = chunkDataOffset;
      dataSize = chunkSize;
    }

    chunkOffset += 8 + chunkSize + (chunkSize % 2);
  }

  if (!audioFormat || !numChannels || !sampleRate || !blockAlign || !bitsPerSample) {
    throw new Error("WAV fmt chunk is missing required fields.");
  }

  if (dataOffset < 0 || dataSize <= 0) {
    throw new Error("WAV data chunk was not found.");
  }

  const bytesPerSample = bitsPerSample / 8;
  const frameCount = Math.floor(dataSize / blockAlign);
  const samples = new Float32Array(frameCount);

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    const frameOffset = dataOffset + frameIndex * blockAlign;
    let monoAccumulator = 0;

    for (let channelIndex = 0; channelIndex < numChannels; channelIndex += 1) {
      const sampleOffset = frameOffset + channelIndex * bytesPerSample;
      monoAccumulator += readWaveSample(view, sampleOffset, audioFormat, bitsPerSample);
    }

    samples[frameIndex] = clampUnitSample(monoAccumulator / numChannels);
  }

  return { sampleRate, samples };
}

/**
 * Read one normalized sample from the WAV data chunk for the requested format.
 * Inputs: view - DataView over the WAV bytes, offset - byte position for the sample,
 * audioFormat - WAV format code, bitsPerSample - source bit depth.
 * Outputs: floating-point sample in the unit range.
 */
function readWaveSample(view, offset, audioFormat, bitsPerSample) {
  if (audioFormat === 1) {
    if (bitsPerSample === 8) {
      return (view.getUint8(offset) - 128) / 128;
    }

    if (bitsPerSample === 16) {
      return view.getInt16(offset, true) / 32768;
    }

    if (bitsPerSample === 24) {
      let sample =
        view.getUint8(offset) |
        (view.getUint8(offset + 1) << 8) |
        (view.getUint8(offset + 2) << 16);

      if (sample & 0x800000) {
        sample |= 0xff000000;
      }

      return sample / 8388608;
    }

    if (bitsPerSample === 32) {
      return view.getInt32(offset, true) / 2147483648;
    }
  }

  if (audioFormat === 3) {
    if (bitsPerSample === 32) {
      return clampUnitSample(view.getFloat32(offset, true));
    }

    if (bitsPerSample === 64) {
      return clampUnitSample(view.getFloat64(offset, true));
    }
  }

  throw new Error(`Unsupported WAV sample format: format=${audioFormat}, bits=${bitsPerSample}`);
}

/**
 * Decode a short ASCII string from a WAV header field.
 * Inputs: view - DataView over the WAV bytes, offset - byte offset, length - byte count.
 * Outputs: ASCII string read from the requested byte range.
 */
function readAscii(view, offset, length) {
  let result = "";

  for (let index = 0; index < length; index += 1) {
    result += String.fromCharCode(view.getUint8(offset + index));
  }

  return result;
}

/**
 * Convert raw waveform samples into a DC-corrected, peak-normalized 256-point cycle.
 * Inputs: samples - decoded mono waveform, pointCount - desired output cycle length.
 * Outputs: Float32Array containing one normalized cycle prepared for playback/export.
 */
function prepareCyclePoints(samples, pointCount) {
  const centeredSamples = removeDcOffsetFloat32(samples);
  const resampledCycle = periodicResampleFloat32(centeredSamples, pointCount);
  return normalizePeakFloat32(resampledCycle);
}

/**
 * Remove the mean value from a waveform.
 * Inputs: samples - Float32Array or array of waveform samples.
 * Outputs: Float32Array containing the same waveform centered around zero.
 */
function removeDcOffsetFloat32(samples) {
  let meanValue = 0;
  for (let index = 0; index < samples.length; index += 1) {
    meanValue += samples[index];
  }
  meanValue /= samples.length;

  const centered = new Float32Array(samples.length);
  for (let index = 0; index < samples.length; index += 1) {
    centered[index] = samples[index] - meanValue;
  }

  return centered;
}

/**
 * Normalize a waveform so its largest absolute sample reaches unity.
 * Inputs: samples - waveform samples that should be scaled for consistent playback.
 * Outputs: Float32Array containing peak-normalized samples.
 */
function normalizePeakFloat32(samples) {
  let peakValue = 0;

  for (let index = 0; index < samples.length; index += 1) {
    peakValue = Math.max(peakValue, Math.abs(samples[index]));
  }

  if (peakValue <= 0) {
    return new Float32Array(samples);
  }

  const normalized = new Float32Array(samples.length);
  for (let index = 0; index < samples.length; index += 1) {
    normalized[index] = samples[index] / peakValue;
  }

  return normalized;
}

/**
 * Resample one periodic cycle to a new point count using wraparound interpolation.
 * Inputs: samples - source cycle samples, pointCount - desired output length.
 * Outputs: Float32Array with pointCount evenly spaced samples over the cycle.
 */
function periodicResampleFloat32(samples, pointCount) {
  const resampled = new Float32Array(pointCount);
  const sourceCount = samples.length;

  for (let pointIndex = 0; pointIndex < pointCount; pointIndex += 1) {
    const sourcePosition = (pointIndex * sourceCount) / pointCount;
    const sourceIndex = Math.floor(sourcePosition) % sourceCount;
    const sourceFraction = sourcePosition - Math.floor(sourcePosition);
    const nextIndex = (sourceIndex + 1) % sourceCount;
    const sampleA = samples[sourceIndex];
    const sampleB = samples[nextIndex];
    resampled[pointIndex] = sampleA + (sampleB - sampleA) * sourceFraction;
  }

  return resampled;
}

/**
 * Convert the quantized preview samples stored in the manifest into draw-ready floats.
 * Inputs: entry - manifest entry containing preview_points_q7.
 * Outputs: Float32Array with preview samples dequantized into [-1, 1].
 */
function getEntryPreviewPoints(entry) {
  return dequantizeQ7(entry.preview_points_q7 || []);
}

/**
 * Return the best available waveform points for drawing one entry.
 * Inputs: entryId - manifest ID for a library or slot wave.
 * Outputs: Float32Array containing either cached 256-point data or manifest preview data.
 */
function getEntryWavePointsForDraw(entryId) {
  const cachedWave = state.waveCache.get(entryId);
  if (cachedWave) {
    return cachedWave.points256;
  }

  const entry = getEntryById(entryId);
  return entry ? getEntryPreviewPoints(entry) : new Float32Array(0);
}

/**
 * Convert preview integers from the manifest into floating-point samples.
 * Inputs: quantizedPoints - integer preview array in roughly [-127, 127].
 * Outputs: Float32Array containing floating-point preview samples.
 */
function dequantizeQ7(quantizedPoints) {
  const points = new Float32Array(quantizedPoints.length);
  for (let index = 0; index < quantizedPoints.length; index += 1) {
    points[index] = quantizedPoints[index] / 127;
  }
  return points;
}

/**
 * Count how many of the 16 slots are currently filled.
 * Inputs: none.
 * Outputs: integer number of non-empty slot objects.
 */
function getFilledSlotCount() {
  return state.slots.filter((slot) => slot !== null).length;
}

/**
 * Warm the cached 256-point cycle for a selected entry in the background.
 * Inputs: entryId - manifest ID that should be ready for fast preview.
 * Outputs: Promise<void>. Cache population happens quietly and errors only update status.
 */
async function warmEntryCycle(entryId) {
  try {
    await ensureEntryCycleLoaded(entryId);
  } catch (error) {
    setStatusMessage(`Could not cache ${getEntryById(entryId)?.display_name || "wave"}: ${String(error.message || error)}`);
  }
}

/**
 * Render a mono audition tone by repeatedly sampling one waveform cycle.
 * Inputs: cyclePoints - normalized cycle samples, frequencyHz - target pitch,
 * durationSeconds - playback duration, sampleRate - output sample rate.
 * Outputs: Float32Array containing the rendered audition audio.
 */
function renderToneBuffer(cyclePoints, frequencyHz, durationSeconds, sampleRate) {
  const sampleCount = Math.max(1, Math.floor(durationSeconds * sampleRate));
  const buffer = new Float32Array(sampleCount);
  const phaseIncrement = (frequencyHz * cyclePoints.length) / sampleRate;
  let phase = 0;

  for (let sampleIndex = 0; sampleIndex < sampleCount; sampleIndex += 1) {
    buffer[sampleIndex] = sampleCycleAtPhase(cyclePoints, phase) * 0.6;
    phase += phaseIncrement;
    while (phase >= cyclePoints.length) {
      phase -= cyclePoints.length;
    }
  }

  applyFadeEnvelope(buffer, sampleRate);
  return buffer;
}

/**
 * Render a morph sweep across multiple slot cycles using linear wavetable interpolation.
 * Inputs: cycles - ordered array of slot waveforms, frequencyHz - fixed audition pitch,
 * durationSeconds - playback duration, sampleRate - output sample rate.
 * Outputs: Float32Array containing the rendered morph audio.
 */
function renderMorphBuffer(cycles, frequencyHz, durationSeconds, sampleRate) {
  const sampleCount = Math.max(1, Math.floor(durationSeconds * sampleRate));
  const buffer = new Float32Array(sampleCount);
  const phaseIncrement = (frequencyHz * CYCLE_POINT_COUNT) / sampleRate;
  let phase = 0;

  for (let sampleIndex = 0; sampleIndex < sampleCount; sampleIndex += 1) {
    const morphPosition = sampleCount > 1
      ? (sampleIndex / (sampleCount - 1)) * (cycles.length - 1)
      : 0;
    const morphIndex = Math.min(cycles.length - 1, Math.floor(morphPosition));
    const nextMorphIndex = Math.min(cycles.length - 1, morphIndex + 1);
    const morphFraction = morphPosition - morphIndex;
    const sampleA = sampleCycleAtPhase(cycles[morphIndex], phase);
    const sampleB = sampleCycleAtPhase(cycles[nextMorphIndex], phase);

    buffer[sampleIndex] = (sampleA + (sampleB - sampleA) * morphFraction) * 0.6;
    phase += phaseIncrement;
    while (phase >= CYCLE_POINT_COUNT) {
      phase -= CYCLE_POINT_COUNT;
    }
  }

  applyFadeEnvelope(buffer, sampleRate);
  return buffer;
}

/**
 * Sample a cycle waveform at a fractional phase position using wraparound interpolation.
 * Inputs: cyclePoints - waveform cycle samples, phase - current phase in table-index units.
 * Outputs: floating-point sample value at the requested phase.
 */
function sampleCycleAtPhase(cyclePoints, phase) {
  const pointCount = cyclePoints.length;
  const baseIndex = Math.floor(phase) % pointCount;
  const nextIndex = (baseIndex + 1) % pointCount;
  const phaseFraction = phase - Math.floor(phase);
  const sampleA = cyclePoints[baseIndex];
  const sampleB = cyclePoints[nextIndex];
  return sampleA + (sampleB - sampleA) * phaseFraction;
}

/**
 * Apply short fades at the start and end of a rendered audio buffer to avoid clicks.
 * Inputs: samples - rendered mono audio, sampleRate - output sample rate.
 * Outputs: none. The input buffer is modified in place.
 */
function applyFadeEnvelope(samples, sampleRate) {
  const fadeSampleCount = Math.min(Math.floor(sampleRate * 0.01), Math.floor(samples.length / 2));

  for (let fadeIndex = 0; fadeIndex < fadeSampleCount; fadeIndex += 1) {
    const gain = fadeIndex / Math.max(1, fadeSampleCount);
    samples[fadeIndex] *= gain;
    samples[samples.length - 1 - fadeIndex] *= gain;
  }
}

/**
 * Play a mono Float32Array buffer through the shared Web Audio output.
 * Inputs: samples - rendered mono audio, sampleRate - buffer playback sample rate.
 * Outputs: none. Existing playback stops and the new buffer begins immediately.
 */
function playMonoBuffer(samples, sampleRate) {
  stopPlayback();

  const audioBuffer = state.audioContext.createBuffer(1, samples.length, sampleRate);
  audioBuffer.copyToChannel(samples, 0);

  const source = state.audioContext.createBufferSource();
  const gain = state.audioContext.createGain();
  gain.gain.value = 0.9;
  source.buffer = audioBuffer;
  source.connect(gain);
  gain.connect(state.audioContext.destination);
  source.start();

  source.addEventListener("ended", () => {
    if (state.activeSource === source) {
      state.activeSource = null;
      updateActionButtons();
    }
  });

  state.activeSource = source;
  state.activeGain = gain;
  updateActionButtons();
}

/**
 * Export the current 16-slot selection as a JSON file with slot metadata and 256-point cycles.
 * Inputs: none.
 * Outputs: Promise<void>. A JSON download is triggered in the browser.
 */
async function exportSelectionJson() {
  const slotPayloads = await Promise.all(
    state.slots.map(async (slot, slotIndex) => {
      if (!slot) {
        return {
          slot_index: slotIndex,
          empty: true,
          cycle_points_q15_256: Array.from({ length: CYCLE_POINT_COUNT }, () => 0),
        };
      }

      const entry = getEntryById(slot.entryId);
      const loadedWave = await ensureEntryCycleLoaded(slot.entryId);
      return {
        slot_index: slotIndex,
        empty: false,
        id: entry.id,
        display_name: entry.display_name,
        file_name: entry.file_name,
        category: entry.category,
        relative_path: entry.relative_path,
        zero_crossings: entry.zero_crossings,
        brightness_score: entry.brightness_score,
        odd_harmonic_ratio: entry.odd_harmonic_ratio,
        dominant_harmonic: entry.dominant_harmonic,
        cycle_points_q15_256: Array.from(loadedWave.points256, quantizeQ15),
      };
    }),
  );

  const payload = {
    exported_at_epoch: Math.floor(Date.now() / 1000),
    slot_count: TARGET_SLOT_COUNT,
    point_count: CYCLE_POINT_COUNT,
    slots: slotPayloads,
  };

  downloadBlob(
    new Blob([JSON.stringify(payload, null, 2)], { type: "application/json" }),
    "floatable_selection_16.json",
  );
  setStatusMessage("Exported selection JSON.");
}

/**
 * Export the current 16-slot bank as a concatenated 16-cycle WAV file.
 * Inputs: none.
 * Outputs: Promise<void>. A WAV download is triggered in the browser.
 */
async function exportBankWav() {
  const cycleSets = await Promise.all(
    state.slots.map(async (slot) => {
      if (!slot) {
        return new Float32Array(CYCLE_POINT_COUNT);
      }
      const loadedWave = await ensureEntryCycleLoaded(slot.entryId);
      return loadedWave.points256;
    }),
  );

  const bankSamples = new Float32Array(TARGET_SLOT_COUNT * CYCLE_POINT_COUNT);
  cycleSets.forEach((cyclePoints, slotIndex) => {
    bankSamples.set(cyclePoints, slotIndex * CYCLE_POINT_COUNT);
  });

  const wavBytes = encodeWavFile(bankSamples, 48000);
  downloadBlob(
    new Blob([wavBytes], { type: "audio/wav" }),
    "floatable_bank_16x256.wav",
  );
  setStatusMessage("Exported 16-slot bank WAV.");
}

/**
 * Encode a mono Float32Array into a 16-bit PCM WAV file.
 * Inputs: samples - mono samples in [-1, 1], sampleRate - WAV sample rate field.
 * Outputs: ArrayBuffer containing a complete RIFF/WAVE file.
 */
function encodeWavFile(samples, sampleRate) {
  const dataLength = samples.length * 2;
  const buffer = new ArrayBuffer(44 + dataLength);
  const view = new DataView(buffer);

  writeAscii(view, 0, "RIFF");
  view.setUint32(4, 36 + dataLength, true);
  writeAscii(view, 8, "WAVE");
  writeAscii(view, 12, "fmt ");
  view.setUint32(16, 16, true);
  view.setUint16(20, 1, true);
  view.setUint16(22, 1, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sampleRate * 2, true);
  view.setUint16(32, 2, true);
  view.setUint16(34, 16, true);
  writeAscii(view, 36, "data");
  view.setUint32(40, dataLength, true);

  for (let sampleIndex = 0; sampleIndex < samples.length; sampleIndex += 1) {
    const clampedSample = clampUnitSample(samples[sampleIndex]);
    const pcmValue = clampedSample < 0
      ? Math.round(clampedSample * 32768)
      : Math.round(clampedSample * 32767);
    view.setInt16(44 + sampleIndex * 2, pcmValue, true);
  }

  return buffer;
}

/**
 * Write an ASCII string into a DataView for WAV header construction.
 * Inputs: view - target DataView, offset - byte position, text - ASCII string to write.
 * Outputs: none. The DataView is modified in place.
 */
function writeAscii(view, offset, text) {
  for (let index = 0; index < text.length; index += 1) {
    view.setUint8(offset + index, text.charCodeAt(index));
  }
}

/**
 * Trigger a browser download for a Blob payload.
 * Inputs: blob - file payload, fileName - suggested output file name.
 * Outputs: none. A temporary object URL is created and clicked.
 */
function downloadBlob(blob, fileName) {
  const objectUrl = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = objectUrl;
  anchor.download = fileName;
  anchor.click();
  URL.revokeObjectURL(objectUrl);
}

/**
 * Draw one waveform into a canvas using the shared display style.
 * Inputs: canvas - target canvas element, points - waveform points to draw, accentColor - stroke color.
 * Outputs: none. The canvas is resized and redrawn.
 */
function drawWaveCanvas(canvas, points, accentColor) {
  const preparedCanvas = prepareCanvas(canvas);
  const { ctx, width, height } = preparedCanvas;

  ctx.clearRect(0, 0, width, height);
  drawCanvasBackdrop(ctx, width, height);

  if (!points || points.length === 0) {
    drawCanvasMessage(ctx, width, height, "No wave");
    return;
  }

  drawWaveformTrace(ctx, points, width, height, accentColor);
}

/**
 * Draw an empty-state message into a canvas by element reference.
 * Inputs: canvas - target canvas element, message - user-facing empty-state text.
 * Outputs: none. The target canvas is cleared and redrawn.
 */
function drawEmptyCanvasState(canvas, message) {
  const preparedCanvas = prepareCanvas(canvas);
  const { ctx, width, height } = preparedCanvas;
  ctx.clearRect(0, 0, width, height);
  drawCanvasBackdrop(ctx, width, height);
  drawCanvasMessage(ctx, width, height, message);
}

/**
 * Prepare a canvas for HiDPI drawing and return its 2D rendering context.
 * Inputs: canvas - HTMLCanvasElement that should be sized to its display box.
 * Outputs: object containing the context and CSS pixel dimensions.
 */
function prepareCanvas(canvas) {
  const devicePixelRatio = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  const width = Math.max(1, Math.round(rect.width));
  const height = Math.max(1, Math.round(rect.height));
  const scaledWidth = Math.round(width * devicePixelRatio);
  const scaledHeight = Math.round(height * devicePixelRatio);

  if (canvas.width !== scaledWidth || canvas.height !== scaledHeight) {
    canvas.width = scaledWidth;
    canvas.height = scaledHeight;
  }

  const ctx = canvas.getContext("2d");
  ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
  return { ctx, width, height };
}

/**
 * Paint the common soft panel background used under waveform drawings.
 * Inputs: ctx - 2D drawing context, width - canvas width, height - canvas height.
 * Outputs: none. The current canvas receives a rounded background fill.
 */
function drawCanvasBackdrop(ctx, width, height) {
  const backgroundGradient = ctx.createLinearGradient(0, 0, 0, height);
  backgroundGradient.addColorStop(0, "rgba(255, 255, 255, 0.94)");
  backgroundGradient.addColorStop(1, "rgba(249, 244, 235, 0.96)");

  ctx.fillStyle = backgroundGradient;
  roundRect(ctx, 0, 0, width, height, 12);
  ctx.fill();
}

/**
 * Draw a waveform trace with baseline and vertical grid marks.
 * Inputs: ctx - 2D drawing context, points - waveform samples, width - canvas width,
 * height - canvas height, accentColor - waveform stroke color.
 * Outputs: none. The supplied waveform trace is painted into the canvas.
 */
function drawWaveformTrace(ctx, points, width, height, accentColor) {
  const top = 12;
  const bottom = height - 12;
  const drawHeight = bottom - top;
  const centerY = height * 0.5;

  ctx.strokeStyle = "rgba(23, 32, 45, 0.08)";
  ctx.lineWidth = 1;
  ctx.beginPath();
  for (let gridIndex = 1; gridIndex < 4; gridIndex += 1) {
    const gridX = (width / 4) * gridIndex;
    ctx.moveTo(gridX + 0.5, 0);
    ctx.lineTo(gridX + 0.5, height);
  }
  ctx.stroke();

  ctx.strokeStyle = "rgba(23, 32, 45, 0.14)";
  ctx.beginPath();
  ctx.moveTo(0, centerY);
  ctx.lineTo(width, centerY);
  ctx.stroke();

  ctx.strokeStyle = accentColor;
  ctx.lineWidth = 2;
  ctx.beginPath();

  points.forEach((sample, pointIndex) => {
    const normalizedX = points.length > 1 ? pointIndex / (points.length - 1) : 0;
    const x = normalizedX * width;
    const y = top + ((1 - (sample + 1) * 0.5) * drawHeight);

    if (pointIndex === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });

  ctx.stroke();
}

/**
 * Draw a centered text message into a prepared canvas.
 * Inputs: ctx - 2D drawing context, width - canvas width, height - canvas height, message - display text.
 * Outputs: none. The message is written into the center of the canvas.
 */
function drawCanvasMessage(ctx, width, height, message) {
  ctx.fillStyle = "rgba(66, 80, 98, 0.88)";
  ctx.font = '14px "Menlo", "Consolas", monospace';
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(message, width * 0.5, height * 0.5);
  ctx.textAlign = "start";
}

/**
 * Draw a rounded rectangle path into the current 2D context.
 * Inputs: ctx - 2D drawing context, x - left position, y - top position,
 * width - rectangle width, height - rectangle height, radius - corner radius.
 * Outputs: none. The current path on the context becomes a rounded rectangle.
 */
function roundRect(ctx, x, y, width, height, radius) {
  ctx.beginPath();
  ctx.moveTo(x + radius, y);
  ctx.lineTo(x + width - radius, y);
  ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
  ctx.lineTo(x + width, y + height - radius);
  ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
  ctx.lineTo(x + radius, y + height);
  ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
  ctx.lineTo(x, y + radius);
  ctx.quadraticCurveTo(x, y, x + radius, y);
  ctx.closePath();
}

/**
 * Create a placeholder card used when no manifest data or results are available.
 * Inputs: message - user-facing empty-state text.
 * Outputs: HTMLElement placeholder card ready to insert into the DOM.
 */
function createPlaceholderCard(message) {
  const card = document.createElement("div");
  card.className = "placeholder-card";
  card.textContent = message;
  return card;
}

/**
 * Create a compact metadata chip used in the library results and candidate panel.
 * Inputs: label - short text to display inside the chip.
 * Outputs: HTMLElement chip span with the given label.
 */
function createChip(label) {
  const chip = document.createElement("span");
  chip.className = "chip";
  chip.textContent = label;
  return chip;
}

/**
 * Create a compact slot metric pill for the slot board cards.
 * Inputs: label - short text to display inside the metric pill.
 * Outputs: HTMLElement pill span with the given metric text.
 */
function createMetricPill(label) {
  const pill = document.createElement("span");
  pill.className = "metric-pill";
  pill.textContent = label;
  return pill;
}

/**
 * Read the current preview pitch control and return a safe positive frequency.
 * Inputs: none.
 * Outputs: number in hertz for candidate, slot, and morph audition playback.
 */
function getPreviewFrequencyHz() {
  const value = Number(dom.previewPitchInput.value);
  if (!Number.isFinite(value) || value <= 0) {
    return DEFAULT_PREVIEW_FREQUENCY_HZ;
  }
  return value;
}

/**
 * Read the current morph duration control and return a safe positive duration.
 * Inputs: none.
 * Outputs: number in seconds for morph sweep audition playback.
 */
function getMorphDurationSeconds() {
  const value = Number(dom.morphDurationInput.value);
  if (!Number.isFinite(value) || value <= 0) {
    return DEFAULT_MORPH_DURATION_SECONDS;
  }
  return value;
}

/**
 * Clamp one floating-point sample into the unit audio range.
 * Inputs: value - numeric sample that may exceed [-1, 1].
 * Outputs: number clamped to the unit range.
 */
function clampUnitSample(value) {
  return Math.max(-1, Math.min(1, value));
}

/**
 * Quantize one floating-point sample into signed 16-bit integer range.
 * Inputs: value - floating-point sample expected near [-1, 1].
 * Outputs: signed integer suitable for export JSON or WAV generation.
 */
function quantizeQ15(value) {
  const clampedValue = clampUnitSample(value);
  return clampedValue < 0
    ? Math.round(clampedValue * 32768)
    : Math.round(clampedValue * 32767);
}

/**
 * Format an integer with grouped separators for UI readability.
 * Inputs: value - integer-like numeric value.
 * Outputs: localized integer string such as "4,358".
 */
function formatInteger(value) {
  return Number(value).toLocaleString("en-US", { maximumFractionDigits: 0 });
}

/**
 * Format a decimal value with up to three fractional digits.
 * Inputs: value - numeric value that may contain a fractional part.
 * Outputs: concise decimal string for UI metrics.
 */
function formatDecimal(value) {
  return Number(value).toLocaleString("en-US", {
    minimumFractionDigits: 0,
    maximumFractionDigits: 3,
  });
}

/**
 * Format a signed decimal metric with explicit sign when positive.
 * Inputs: value - numeric value that may be positive or negative.
 * Outputs: signed decimal string such as "+0.012" or "-0.008".
 */
function formatSignedDecimal(value) {
  const numericValue = Number(value);
  const prefix = numericValue > 0 ? "+" : "";
  return `${prefix}${formatDecimal(numericValue)}`;
}

/**
 * Format a 0..1 ratio as a whole-number percentage string.
 * Inputs: value - numeric ratio expected near the unit range.
 * Outputs: percentage string such as "42%".
 */
function formatPercent(value) {
  return `${Math.round(Number(value) * 100)}%`;
}

/**
 * Apply a deterministic demo selection when the page is opened with ?demo=1.
 * Inputs: none.
 * Outputs: none. The first 16 filtered entries are slotted for quick verification.
 */
function applyDemoSelectionIfRequested() {
  const searchParams = new URLSearchParams(window.location.search);
  if (searchParams.get("demo") !== "1") {
    return;
  }

  const sourceEntries = state.filteredEntries.length >= TARGET_SLOT_COUNT
    ? state.filteredEntries
    : Array.isArray(state.manifest?.waves) ? state.manifest.waves : [];

  replaceSlotsFromEntries(sourceEntries.slice(0, TARGET_SLOT_COUNT));
  setStatusMessage("Applied demo slot selection from the first 16 available waves.");
}

document.addEventListener("DOMContentLoaded", initializeApp);
