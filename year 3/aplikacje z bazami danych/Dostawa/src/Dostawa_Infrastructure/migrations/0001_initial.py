# Generated by Django 3.0.6 on 2020-05-30 14:55

import Dostawa_Domain.Model.DeliveryType.DeliveryType
import Dostawa_Domain.Model.Package.Package
import Dostawa_Domain.Model.Package.ValueObjects.Pickup
import Dostawa_Domain.Model.Package.ValueObjects.Return
from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='DeliveryTypeModel',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('_NameField', models.CharField(max_length=200)),
                ('_PriceField', models.PositiveIntegerField()),
                ('_DeliveryTimeField', models.CharField(max_length=200)),
            ],
            bases=(Dostawa_Domain.Model.DeliveryType.DeliveryType.DeliveryType, models.Model),
        ),
        migrations.CreateModel(
            name='PackageModel',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('_CityField', models.CharField(max_length=50)),
                ('_PostalCodeField', models.CharField(max_length=20)),
                ('_StreetAddressField', models.CharField(max_length=200)),
                ('_ClientIdField', models.PositiveIntegerField()),
                ('_DeclaredValueField', models.PositiveIntegerField()),
                ('_AcceptDateField', models.DateField()),
                ('_DeliveryDateField', models.DateField(null=True)),
                ('_PackageCodeField', models.PositiveIntegerField()),
                ('_StatusNameField', models.CharField(max_length=100)),
                ('_StatusDeliveryStepField', models.PositiveSmallIntegerField()),
            ],
            bases=(Dostawa_Domain.Model.Package.Package.Package, models.Model),
        ),
        migrations.CreateModel(
            name='ReturnModel',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('_DescriptionField', models.CharField(max_length=200)),
                ('_SumField', models.PositiveIntegerField()),
                ('_ReportDateField', models.DateField()),
                ('_AcceptedField', models.BooleanField(default=False)),
                ('_StatusField', models.CharField(max_length=100)),
            ],
            bases=(Dostawa_Domain.Model.Package.ValueObjects.Return.Return, models.Model),
        ),
        migrations.CreateModel(
            name='PickupModel',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('_NameField', models.CharField(max_length=200)),
                ('_AmountField', models.PositiveIntegerField(default=1)),
                ('_IsPackedField', models.BooleanField(default=False)),
                ('_PackingDateField', models.DateField(default=None, null=True)),
                ('_PackageForeignKey', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='Dostawa_Infrastructure.PackageModel')),
            ],
            bases=(Dostawa_Domain.Model.Package.ValueObjects.Pickup.Pickup, models.Model),
        ),
        migrations.AddField(
            model_name='packagemodel',
            name='_ReturnForeignKey',
            field=models.ForeignKey(default=None, null=True, on_delete=django.db.models.deletion.CASCADE, to='Dostawa_Infrastructure.ReturnModel'),
        ),
    ]
