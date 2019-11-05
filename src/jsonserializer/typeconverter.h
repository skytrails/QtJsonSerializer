#ifndef QTJSONSERIALIZER_TYPECONVERTER_H
#define QTJSONSERIALIZER_TYPECONVERTER_H

#include "QtJsonSerializer/qtjsonserializer_global.h"

#include <type_traits>
#include <limits>

#include <QtCore/qmetatype.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qcborvalue.h>
#include <QtCore/qvariant.h>
#include <QtCore/qsharedpointer.h>

namespace QtJsonSerializer {

class Q_JSONSERIALIZER_EXPORT TypeExtractor
{
	Q_DISABLE_COPY(TypeExtractor)

public:
	TypeExtractor();
	virtual ~TypeExtractor();

	virtual QByteArray baseType() const = 0;
	virtual QList<int> subtypes() const = 0;
	virtual QVariant extract(const QVariant &value, int index = -1) const = 0;
	virtual void emplace(QVariant &target, const QVariant &value, int index = -1) const = 0;
};

class TypeConverterPrivate;
//! An interface to create custom serializer type converters
class Q_JSONSERIALIZER_EXPORT TypeConverter
{
	Q_DISABLE_COPY(TypeConverter)
public:
	static constexpr auto NoTag = static_cast<QCborTag>(std::numeric_limits<std::underlying_type_t<QCborTag>>::max());

	//! Sample values for a priority value (default converters are mostly Standard and are guaranteed to be between Low and High)
	enum Priority : int {
		ExtremlyLow = -0x00FFFFFF,
		VeryLow = -0x0000FFFF,
		Low = -0x000000FF,
		Standard = 0x00000000,
		High = 0x000000FF,
		VeryHigh = 0x0000FFFF,
		ExtremlyHigh = 0x00FFFFFF
	};

	enum DeserializationCapabilityResult : int {
		Positive = 1,
		Guessed = 2,
		Negative = -1,
		WrongTag = -2
	};

	//! Helper class passed to the type converter by the serializer. Do not implement yourself
	class Q_JSONSERIALIZER_EXPORT SerializationHelper
	{
		Q_DISABLE_COPY(SerializationHelper)
	public:
		SerializationHelper();
		virtual ~SerializationHelper();

		virtual bool jsonMode() const = 0;
		//! Returns a property from the serializer
		virtual QVariant getProperty(const char *name) const = 0;
		virtual QCborTag typeTag(int metaTypeId) const = 0;
		virtual QSharedPointer<const TypeExtractor> extractor(int metaTypeId) const = 0;

		//! Serialize a subvalue, represented by a meta property
		virtual QCborValue serializeSubtype(const QMetaProperty &property, const QVariant &value) const = 0;
		//! Serialize a subvalue, represented by a type id
		virtual QCborValue serializeSubtype(int propertyType, const QVariant &value, const QByteArray &traceHint = {}) const = 0;
		//! Deserialize a subvalue, represented by a meta property
		virtual QVariant deserializeSubtype(const QMetaProperty &property, const QCborValue &value, QObject *parent) const = 0;
		//! Deserialize a subvalue, represented by a type id
		virtual QVariant deserializeSubtype(int propertyType, const QCborValue &value, QObject *parent, const QByteArray &traceHint = {}) const = 0;
	};

	//! Constructor
	TypeConverter();
	//! Destructor
	virtual ~TypeConverter();

	//! Returns the priority of this converter
	int priority() const;
	//! Sets the priority of this converter
	void setPriority(int priority);

	const SerializationHelper *helper() const;
	void setHelper(const SerializationHelper *helper);

	//! Returns true, if this implementation can convert the given type
	virtual bool canConvert(int metaTypeId) const = 0;
	virtual QList<QCborTag> allowedCborTags(int metaTypeId) const;
	virtual QList<QCborValue::Type> allowedCborTypes(int metaTypeId, QCborTag tag) const = 0;
	virtual int guessType(QCborTag tag, QCborValue::Type dataType) const;
	DeserializationCapabilityResult canDeserialize(int &metaTypeId,
												   QCborTag tag,
												   QCborValue::Type dataType) const;

	//! Called by the serializer to serializer your given type
	virtual QCborValue serialize(int propertyType, const QVariant &value) const = 0;
	//! Called by the deserializer to serializer your given type
	virtual QVariant deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const = 0;
	virtual QVariant deserializeJson(int propertyType, const QCborValue &value, QObject *parent) const;

private:
	QScopedPointer<TypeConverterPrivate> d;

	void mapTypesToJson(QList<QCborValue::Type> &typeList) const;
};

//! A factory interface to create instances of QJsonTypeConverters
class Q_JSONSERIALIZER_EXPORT TypeConverterFactory
{
	Q_DISABLE_COPY(TypeConverterFactory)

public:
	TypeConverterFactory();
	virtual ~TypeConverterFactory();

	//! The primary factory method to create converters
	virtual QSharedPointer<TypeConverter> createConverter() const = 0;
};

//! A template implementation of QJsonTypeConverterFactory to generically create simply converters
template <typename TConverter, int OverwritePriority = TypeConverter::Priority::Standard>
class TypeConverterStandardFactory : public TypeConverterFactory
{
public:
	QSharedPointer<TypeConverter> createConverter() const override;
};

// ------------- GENERIC IMPLEMENTATION -------------

template<typename TConverter, int OverwritePriority>
QSharedPointer<TypeConverter> TypeConverterStandardFactory<TConverter, OverwritePriority>::createConverter() const
{
	auto converter = QSharedPointer<TConverter>::create();
	if (OverwritePriority != TypeConverter::Priority::Standard)
		converter->setPriority(OverwritePriority);
	return converter;
}

}

#endif // QTJSONSERIALIZER_TYPECONVERTER_H