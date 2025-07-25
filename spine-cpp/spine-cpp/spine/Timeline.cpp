/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated April 5, 2025. Replaces all prior versions.
 *
 * Copyright (c) 2013-2025, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <spine/Timeline.h>

#include <spine/Event.h>
#include <spine/Skeleton.h>

namespace spine {
	RTTI_IMPL_NOPARENT(Timeline)

	Timeline::Timeline(size_t frameCount, size_t frameEntries)
		: _propertyIds(), _frames(), _frameEntries(frameEntries) {
		_frames.setSize(frameCount * frameEntries, 0);
	}

	Timeline::~Timeline() {
	}

	Vector<PropertyId> &Timeline::getPropertyIds() {
		return _propertyIds;
	}

	void Timeline::setPropertyIds(PropertyId propertyIds[], size_t propertyIdsCount) {
		_propertyIds.clear();
		_propertyIds.ensureCapacity(propertyIdsCount);
		for (size_t i = 0; i < propertyIdsCount; i++) {
			_propertyIds.add(propertyIds[i]);
		}
	}

	size_t Timeline::getFrameCount() {
		return _frames.size() / _frameEntries;
	}

	Vector<float> &Timeline::getFrames() {
		return _frames;
	}

	size_t Timeline::getFrameEntries() {
		return _frameEntries;
	}

	float Timeline::getDuration() {
		return _frames[_frames.size() - getFrameEntries()];
	}
}// namespace spine
