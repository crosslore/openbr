/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2012 The MITRE Corporation                                      *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <openbr/plugins/openbr_internal.h>

namespace br
{

/*!
 * \ingroup transforms
 * \brief Sets the metadata key/value pair.
 * \author Josh Klontz \cite jklontz
 */
class SetMetadataTransform : public UntrainableMetadataTransform
{
    Q_OBJECT
    Q_PROPERTY(QString key READ get_key WRITE set_key RESET reset_key STORED false)
    Q_PROPERTY(QString value READ get_value WRITE set_value RESET reset_value STORED false)
    BR_PROPERTY(QString, key, "")
    BR_PROPERTY(QString, value, "")

    void projectMetadata(const File &src, File &dst) const
    {
        dst = src;
        dst.set(key, value);
        if (key == "FTE")
            dst.fte = true;
    }
};

BR_REGISTER(Transform, SetMetadataTransform)

} // namespace br

#include "metadata/setmetadata.moc"
